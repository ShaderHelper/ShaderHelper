#include "CommonHeader.h"
#include "SpirvDebugger.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	SpvType* GetAccessedType(const SpvVariable* Var, const TArray<int32>& Indexes)
	{
		SpvType* CurType = Var->Type;
		for (int32 Index : Indexes)
		{
			switch (CurType->GetKind())
			{
			case SpvTypeKind::Vector: CurType = static_cast<SpvVectorType*>(CurType)->ElementType; break;
			case SpvTypeKind::Array:  CurType = static_cast<SpvArrayType*>(CurType)->ElementType;  break;
			case SpvTypeKind::Matrix: CurType = static_cast<SpvMatrixType*>(CurType)->ElementType;  break;
			case SpvTypeKind::Struct: CurType = static_cast<SpvStructType*>(CurType)->MemberTypes[Index]; break;
			default: AUX::Unreachable();
			}
		}
		return CurType;
	}

	TValueOrError<std::tuple<SpvType*, int32>, FString> GetAccess(const SpvVariable* Var, const TArray<int32>& Indexes)
	{
		int32 Offset = 0;
		SpvType* CurType = Var->Type;
		for (int32 Index : Indexes)
		{
			if (CurType->GetKind() == SpvTypeKind::Vector)
			{
				SpvVectorType* VectorType = static_cast<SpvVectorType*>(CurType);
				int32 Increment = Index * GetTypeByteSize(VectorType->ElementType);
				if (Increment >= GetTypeByteSize(VectorType) || Increment < 0)
				{
					return MakeError(FString::Printf(TEXT("Index %d is out of bounds"), Index));
				}
				Offset += Increment;
				CurType = VectorType->ElementType;
			}
			else if (CurType->GetKind() == SpvTypeKind::Struct)
			{
				SpvStructType* StructType = static_cast<SpvStructType*>(CurType);
				for (int32 MemberIndex = 0; MemberIndex < Index; MemberIndex++)
				{
					Offset += GetTypeByteSize(StructType->MemberTypes[MemberIndex]);
				}
				SpvType* MemberType = StructType->MemberTypes[Index];
				CurType = MemberType;
			}
			else if (CurType->GetKind() == SpvTypeKind::Array)
			{
				SpvArrayType* ArrayType = static_cast<SpvArrayType*>(CurType);
				int32 Increment = Index * GetTypeByteSize(ArrayType->ElementType);
				if (Increment >= GetTypeByteSize(ArrayType) || Increment < 0)
				{
					return MakeError(FString::Printf(TEXT("Index %d is out of bounds"), Index));
				}
				Offset += Increment;
				CurType = ArrayType->ElementType;
			}
			else if (CurType->GetKind() == SpvTypeKind::Matrix)
			{
				SpvMatrixType* MatrixType = static_cast<SpvMatrixType*>(CurType);
				int32 Increment = Index * GetTypeByteSize(MatrixType->ElementType);
				if (Increment >= GetTypeByteSize(MatrixType) || Increment < 0)
				{
					return MakeError(FString::Printf(TEXT("Index %d is out of bounds"), Index));
				}
				Offset += Increment;
				CurType = MatrixType->ElementType;
			}
			else
			{
				AUX::Unreachable();
			}
		}
		return MakeValue( CurType,Offset );
	}

	int32 GetInstIndex(const TArray<TUniquePtr<SpvInstruction>>* Insts, SpvId Inst)
	{
		return Insts->IndexOfByPredicate([&](const TUniquePtr<SpvInstruction>& InInst) {
			return InInst->GetId() ? InInst->GetId().value() == Inst : false;
		});
	}

	TPair<int32, int32> ComputeExtractInitUnitRange(SpvType* Type, const TArray<uint32>& Indexes, int32 StartIdx)
	{
		if (StartIdx >= Indexes.Num())
		{
			return { 0, GetTypeByteSize(Type) / 4 };
		}

		uint32 Idx = Indexes[StartIdx];

		switch (Type->GetKind())
		{
		case SpvTypeKind::Vector:
		{
			auto* VecType = static_cast<SpvVectorType*>(Type);
			int32 ElemSize = GetTypeByteSize(VecType->ElementType) / 4;
			auto Sub = ComputeExtractInitUnitRange(VecType->ElementType, Indexes, StartIdx + 1);
			if (Sub.Key < 0) return { -1, 0 };
			return { (int32)Idx * ElemSize + Sub.Key, Sub.Value };
		}
		case SpvTypeKind::Struct:
		{
			auto* StructType = static_cast<SpvStructType*>(Type);
			if ((int32)Idx >= StructType->MemberTypes.Num()) return { -1, 0 };
			int32 Offset = 0;
			for (int32 i = 0; i < (int32)Idx; i++)
			{
				Offset += GetTypeByteSize(StructType->MemberTypes[i]) / 4;
			}
			auto Sub = ComputeExtractInitUnitRange(StructType->MemberTypes[Idx], Indexes, StartIdx + 1);
			if (Sub.Key < 0) return { -1, 0 };
			return { Offset + Sub.Key, Sub.Value };
		}
		case SpvTypeKind::Array:
		{
			auto* ArrType = static_cast<SpvArrayType*>(Type);
			int32 ElemSize = GetTypeByteSize(ArrType->ElementType) / 4;
			auto Sub = ComputeExtractInitUnitRange(ArrType->ElementType, Indexes, StartIdx + 1);
			if (Sub.Key < 0) return { -1, 0 };
			return { (int32)Idx * ElemSize + Sub.Key, Sub.Value };
		}
		case SpvTypeKind::Matrix:
		{
			auto* MatType = static_cast<SpvMatrixType*>(Type);
			int32 ElemSize = GetTypeByteSize(MatType->ElementType) / 4;
			auto Sub = ComputeExtractInitUnitRange(MatType->ElementType, Indexes, StartIdx + 1);
			if (Sub.Key < 0) return { -1, 0 };
			return { (int32)Idx * ElemSize + Sub.Key, Sub.Value };
		}
		default:
			return { -1, 0 };
		}
	}

	// Analyze each OpLoad to determine which init-units (4-byte slots) are actually consumed externally.
	// Returns LoadResultId -> narrowed CheckMask.
	TMap<SpvId, uint32> ComputeLoadCheckMasks(const TArray<TUniquePtr<SpvInstruction>>* Insts, const SpvMetaContext& Context, const SpvPatcher& Patcher)
	{
		TMap<SpvId, uint32> Result;

		int32 EntryIndex = GetInstIndex(Insts, Context.EntryPoint);
		// UseMap: SSA value (raw uint32) -> list of instruction indices that reference it as an operand.
		TMap<uint32, TArray<int32>> UseMap;
		// IdResultType: SSA value -> its result type, used later to resolve vector sizes.
		TMap<uint32, SpvId> IdResultType;

		// a shuffle component index "2" could collide with SSA ID %2 in raw word scan).
		auto AddUse = [&](uint32 Id, int32 InstIdx) { UseMap.FindOrAdd(Id).AddUnique(InstIdx); };
		for (int32 i = EntryIndex; i < Insts->Num(); i++)
		{
			SpvInstruction* Inst = (*Insts)[i].Get();

			if (auto* Load = dynamic_cast<SpvOpLoad*>(Inst))
			{
				IdResultType.Add(Load->GetId().value().GetValue(), Load->GetResultType());
				AddUse(Load->GetPointer().GetValue(), i);
			}
			else if (auto* Store = dynamic_cast<SpvOpStore*>(Inst))
			{
				AddUse(Store->GetPointer().GetValue(), i);
				AddUse(Store->GetObject().GetValue(), i);
			}
			else if (auto* Shuffle = dynamic_cast<SpvOpVectorShuffle*>(Inst))
			{
				IdResultType.Add(Shuffle->GetId().value().GetValue(), Shuffle->GetResultType());
				AddUse(Shuffle->GetVector1().GetValue(), i);
				AddUse(Shuffle->GetVector2().GetValue(), i);
				// Components are literals, not IDs — intentionally skipped
			}
			else if (auto* Extract = dynamic_cast<SpvOpCompositeExtract*>(Inst))
			{
				IdResultType.Add(Extract->GetId().value().GetValue(), Extract->GetResultType());
				AddUse(Extract->GetComposite().GetValue(), i);
				// Indexes are literals, not IDs — intentionally skipped
			}
			else if (auto* Construct = dynamic_cast<SpvOpCompositeConstruct*>(Inst))
			{
				IdResultType.Add(Construct->GetId().value().GetValue(), Construct->GetResultType());
				for (SpvId C : Construct->GetConstituents())
					AddUse(C.GetValue(), i);
			}
			else
			{
				// Unrecognized instruction: conservatively register all words as potential uses.
				// Skip header (word 0) and result type + result id if present (they are definitions, not uses).
				// For ExtInst: [header][ResultType][ResultId][ExtSet][Opcode(literal)][operands...]
				// Also skip ExtSet (import id, not a data value) and the literal opcode at word 4.
				const TArray<uint32>& SpvCode = Patcher.GetSpv();
				int32 Offset = Inst->GetWordOffset().value();
				int32 Len = Inst->GetWordLen().value();
				bool bIsExtInst = std::holds_alternative<SpvGLSLstd450>(Inst->GetKind())
					|| std::holds_alternative<SpvDebugInfo100>(Inst->GetKind());
				int32 Start = bIsExtInst ? Offset + 5
					: (Inst->GetId().has_value() ? Offset + 3 : Offset + 1);
				for (int32 w = Start; w < Offset + Len; w++)
				{
					AddUse(SpvCode[w], i);
				}
			}
		}

		// For each OpLoad, propagate the loaded value through its consumers.
		for (int32 InstIndex = EntryIndex; InstIndex < Insts->Num(); InstIndex++)
		{
			auto* LoadInst = dynamic_cast<SpvOpLoad*>((*Insts)[InstIndex].Get());
			if (!LoadInst)
				continue;

			SpvId LoadResultId = LoadInst->GetId().value();
			SpvId ResultTypeId = LoadInst->GetResultType();

			auto TypeIt = Context.Types.find(ResultTypeId);
			if (TypeIt == Context.Types.end())
				continue;

			SpvType* LoadedType = TypeIt->second.Get();
			int32 TotalUnits = GetTypeByteSize(LoadedType) / 4;
			if (TotalUnits > 32 || LoadedType->IsScalar())
				continue;

			uint32 FullMask = (TotalUnits == 32) ? 0xFFFFFFFFu : ((1u << TotalUnits) - 1u);
			SpvId LoadPointer = LoadInst->GetPointer();

			// CarriesMask: SSA value -> bitmask of init-units from the original Load that this value carries.
			// As values flow through shuffles/extracts, the mask gets narrowed to relevant units.
			TMap<uint32, uint32> CarriesMask;
			CarriesMask.Add(LoadResultId.GetValue(), FullMask);

			// CheckMask accumulates bits for init-units that are externally consumed (not written back).
			uint32 CheckMask = 0;
			// Worklist of SSA values (raw uint32) whose consumers still need analysis.
			TArray<uint32> Worklist;
			Worklist.Add(LoadResultId.GetValue());

			while (Worklist.Num() > 0 && CheckMask != FullMask)
			{
				uint32 ValId = Worklist[0];
				Worklist.RemoveAt(0);
				uint32 ValMask = CarriesMask[ValId];
				if (ValMask == 0)
					continue;

				const TArray<int32>* Users = UseMap.Find(ValId);
				if (!Users)
					continue;

				for (int32 UserIdx : *Users)
				{
					if (CheckMask == FullMask)
						break;

					SpvInstruction* User = (*Insts)[UserIdx].Get();

					if (User->GetId().has_value() && User->GetId().value().GetValue() == ValId)
						continue;

					if (auto* Store = dynamic_cast<SpvOpStore*>(User))
					{
						if (Store->GetObject().GetValue() == ValId)
						{
							// Store back to the same pointer, bits stay unchecked.
							// Store to a different pointer, mark all carried bits.
							if (Store->GetPointer() == LoadPointer)
								continue;
							else
								CheckMask |= ValMask;
						}
						continue;
					}

					// VectorShuffle: track which components of the result originate from the tracked value.
					// Only propagate the bits corresponding to source components that are actually selected.
					if (auto* Shuffle = dynamic_cast<SpvOpVectorShuffle*>(User))
					{
						bool IsVec1 = (Shuffle->GetVector1().GetValue() == ValId);
						bool IsVec2 = (Shuffle->GetVector2().GetValue() == ValId);
						if (!IsVec1 && !IsVec2)
						{
							CheckMask |= ValMask;
							continue;
						}

						uint32 Vec1Size = 0;
						{
							SpvId Vec1Id = Shuffle->GetVector1();
							SpvId Vec1TypeId;
							if (Vec1Id == LoadResultId)
								Vec1TypeId = ResultTypeId;
							else if (SpvId* FoundType = IdResultType.Find(Vec1Id.GetValue()))
								Vec1TypeId = *FoundType;

							auto Vec1TypeIt = Context.Types.find(Vec1TypeId);
							if (Vec1TypeIt != Context.Types.end() && Vec1TypeIt->second->GetKind() == SpvTypeKind::Vector)
							{
								Vec1Size = static_cast<SpvVectorType*>(Vec1TypeIt->second.Get())->ElementCount;
							}
							else
							{
								CheckMask |= ValMask;
								continue;
							}
						}

						uint32 NewMask = 0;
						const TArray<uint32>& Comps = Shuffle->GetComponents();
						for (uint32 ci = 0; ci < (uint32)Comps.Num(); ci++)
						{
							uint32 Comp = Comps[ci];
							if (Comp == 0xFFFFFFFF) continue;

							if (IsVec1 && Comp < Vec1Size)
							{
								if (ValMask & (1u << Comp))
									NewMask |= (1u << Comp);
							}
							else if (IsVec2 && Comp >= Vec1Size)
							{
								uint32 SrcComp = Comp - Vec1Size;
								if (ValMask & (1u << SrcComp))
									NewMask |= (1u << SrcComp);
							}
						}

						if (NewMask != 0)
						{
							SpvId ShuffleId = Shuffle->GetId().value();
							uint32* Existing = CarriesMask.Find(ShuffleId.GetValue());
							uint32 Old = Existing ? *Existing : 0;
							if ((Old | NewMask) != Old)
							{
								CarriesMask.Add(ShuffleId.GetValue(), Old | NewMask);
								Worklist.AddUnique(ShuffleId.GetValue());
							}
						}
						continue;
					}

					// CompositeExtract: narrow the mask to just the init-units covered by the extracted sub-range.
					// E.g. extracting .xy from a mat2 only needs init-units for columns 0-1.
					if (auto* Extract = dynamic_cast<SpvOpCompositeExtract*>(User))
					{
						if (Extract->GetComposite().GetValue() != ValId)
						{
							CheckMask |= ValMask;
							continue;
						}

						SpvId ExtId = Extract->GetId().value();
						uint32 NewMask = ValMask;

						if (ValId == LoadResultId.GetValue() && Extract->GetIndexes().Num() > 0)
						{
							auto Range = ComputeExtractInitUnitRange(LoadedType, Extract->GetIndexes());
							if (Range.Key >= 0)
							{
								uint32 ExtractMask = 0;
								for (int32 u = Range.Key; u < Range.Key + Range.Value && u < 32; u++)
									ExtractMask |= (1u << u);
								NewMask = ValMask & ExtractMask;
							}
						}

						if (NewMask != 0)
						{
							uint32* Existing = CarriesMask.Find(ExtId.GetValue());
							uint32 Old = Existing ? *Existing : 0;
							if ((Old | NewMask) != Old)
							{
								CarriesMask.Add(ExtId.GetValue(), Old | NewMask);
								Worklist.AddUnique(ExtId.GetValue());
							}
						}
						continue;
					}

					// Fallback for unrecognized instructions: conservatively scan raw SPIR-V words.
					// If the tracked value ID appears anywhere in the operands, mark all carried bits as escaped.
					const TArray<uint32>& SpvCode = Patcher.GetSpv();
					int32 Off = User->GetWordOffset().value();
					int32 Ln = User->GetWordLen().value();
					// Skip header, result type and result id — they are definitions, not uses.
					// ExtInst additionally has ExtSet and a literal opcode to skip.
					bool bIsExtInst = std::holds_alternative<SpvGLSLstd450>(User->GetKind())
						|| std::holds_alternative<SpvDebugInfo100>(User->GetKind());
					int32 Start = bIsExtInst ? Off + 5
						: (User->GetId().has_value() ? Off + 3 : Off + 1);
					for (int32 w = Start; w < Off + Ln; w++)
					{
						if (SpvCode[w] == ValId)
						{
							CheckMask |= ValMask;
							break;
						}
					}

				}
			}

			// Only record loads whose CheckMask is narrower than FullMask.
			// These loads have some init-units that are only written back (not externally consumed),
			// so we can skip the uninitialized-access check for those bits.
			if (CheckMask != FullMask)
			{
				Result.Add(LoadResultId, CheckMask);
			}
		}

		return Result;
	}

	void BuildSpvCFG(const TArray<TUniquePtr<SpvInstruction>>* Insts, const SpvMetaContext& Context, std::unordered_map<SpvId, SpvBasicBlock>& OutBBs, std::unordered_map<SpvId, SpvFunc>& OutFuncs)
	{
		int32 EntryIndex = GetInstIndex(Insts, Context.EntryPoint);
		SpvId CurLabel = 0;
		SpvId CurFuncId = 0;
		bool InBlock = false;

		for (int32 i = EntryIndex; i < Insts->Num(); i++)
		{
			if (auto* FuncInst = dynamic_cast<SpvOpFunction*>((*Insts)[i].Get()))
			{
				CurFuncId = FuncInst->GetId().value();
				OutFuncs.emplace(CurFuncId, SpvFunc{FuncInst->GetFunctionType(), FuncInst->GetResultType()});
			}
			else if (auto* ParamInst = dynamic_cast<SpvOpFunctionParameter*>((*Insts)[i].Get()))
			{
				OutFuncs[CurFuncId].Parameters.Add(ParamInst->GetId().value());
			}
			else if (dynamic_cast<SpvOpFunctionEnd*>((*Insts)[i].Get()))
			{
				CurFuncId = 0;
			}
			else if (auto* LabelInst = dynamic_cast<SpvOpLabel*>((*Insts)[i].Get()))
			{
				CurLabel = LabelInst->GetId().value();
				auto& BB = OutBBs.try_emplace(CurLabel, SpvBasicBlock{}).first->second;
				BB.StartIdx = i + 1;
				OutFuncs[CurFuncId].BasicBlocks.Add(CurLabel);
				InBlock = true;
			}
			else if (InBlock)
			{
				bool IsTerminator = false;
				if (auto* Br = dynamic_cast<SpvOpBranch*>((*Insts)[i].Get()))
				{
					OutBBs[CurLabel].Successors.Add(Br->GetTargetLabel());
					IsTerminator = true;
				}
				else if (auto* BrCond = dynamic_cast<SpvOpBranchConditional*>((*Insts)[i].Get()))
				{
					OutBBs[CurLabel].Successors.Add(BrCond->GetTrueLabel());
					OutBBs[CurLabel].Successors.Add(BrCond->GetFalseLabel());
					IsTerminator = true;
				}
				else if (auto* Sw = dynamic_cast<SpvOpSwitch*>((*Insts)[i].Get()))
				{
					OutBBs[CurLabel].Successors.Add(Sw->GetDefault());
					for (const auto& Target : Sw->GetTargets())
						OutBBs[CurLabel].Successors.Add(Target.Value);
					IsTerminator = true;
				}
				else if (dynamic_cast<SpvOpReturn*>((*Insts)[i].Get()) ||
					dynamic_cast<SpvOpReturnValue*>((*Insts)[i].Get()) ||
					dynamic_cast<SpvOpKill*>((*Insts)[i].Get()))
				{
					IsTerminator = true;
				}
				if (IsTerminator)
				{
					OutBBs[CurLabel].EndIdx = i;
					InBlock = false;
				}
			}
		}

		// Build predecessor lists from successor edges
		for (auto& [Label, BB] : OutBBs)
		{
			for (SpvId Succ : BB.Successors)
			{
				if (OutBBs.contains(Succ))
					OutBBs[Succ].Predecessors.Add(Label);
			}
		}
	}

	TSet<SpvId> ComputeStaticallyInitializedVars(const TArray<TUniquePtr<SpvInstruction>>* Insts, const SpvMetaContext& Context, const TMap<SpvId, uint32>& LoadUsedInitUnits, const std::unordered_map<SpvId, SpvBasicBlock>& BBs, const std::unordered_map<SpvId, SpvFunc>& Funcs)
	{
		// Identify local variables that are provably fully initialized before any read,
		// using classic forward dataflow analysis over the CFG.
		//
		// A variable is "statically initialized" if on every path from function entry to
		// any OpLoad of V, there exists an OpStore directly to V (not through OpAccessChain).
		//
		// Variables used as function call arguments are excluded because their initialization
		// state needs runtime propagation to callee parameters.

		TSet<SpvId> Result;
		int32 EntryIndex = GetInstIndex(Insts, Context.EntryPoint);

		// Phase 1: Collect variable IDs used as function call arguments
		TSet<SpvId> FuncCallArgVars;
		for (int32 i = EntryIndex; i < Insts->Num(); i++)
		{
			if (auto* CallInst = dynamic_cast<SpvOpFunctionCall*>((*Insts)[i].Get()))
			{
				for (SpvId Arg : CallInst->GetArguments())
					FuncCallArgVars.Add(Arg);
			}
		}

		// Phase 1.5: Mark compiler-generated temporaries as statically initialized
		for (int32 i = EntryIndex; i < Insts->Num(); i++)
		{
			if (auto* VarInst = dynamic_cast<SpvOpVariable*>((*Insts)[i].Get()))
			{
				if (VarInst->GetStorageClass() == SpvStorageClass::Function)
				{
					SpvId VarId = VarInst->GetId().value();
					if (!Context.VariableDescMap.contains(VarId) && !FuncCallArgVars.Contains(VarId))
						Result.Add(VarId);
				}
			}
		}

		// Phase 2: Per-function CFG-based definite-assignment analysis

		for (const auto& [FuncId, Func] : Funcs)
		{
			const TArray<SpvId>& BlockLabels = Func.BasicBlocks;
			// Collect local variables and AccessChain mappings from this function's blocks
			TSet<SpvId> LocalVars;
			TMap<SpvId, SpvId> ACBaseVar;

			for (SpvId Label : BlockLabels)
			{
				const auto& BB = BBs.at(Label);
				if (BB.EndIdx < 0) continue;
				for (int32 i = BB.StartIdx; i <= BB.EndIdx; i++)
				{
					if (auto* VarInst = dynamic_cast<SpvOpVariable*>((*Insts)[i].Get()))
					{
						if (VarInst->GetStorageClass() == SpvStorageClass::Function)
						{
							SpvId VarId = VarInst->GetId().value();
							if (!Result.Contains(VarId) && !FuncCallArgVars.Contains(VarId))
								LocalVars.Add(VarId);
						}
					}
				}
			}

			if (LocalVars.IsEmpty())
				continue;

			for (SpvId Label : BlockLabels)
			{
				const auto& BB = BBs.at(Label);
				if (BB.EndIdx < 0) continue;
				for (int32 i = BB.StartIdx; i <= BB.EndIdx; i++)
				{
					if (auto* ACInst = dynamic_cast<SpvOpAccessChain*>((*Insts)[i].Get()))
					{
						SpvId Base = ACInst->GetBasePointer();
						if (SpvId* TransitiveBase = ACBaseVar.Find(Base))
							Base = *TransitiveBase;
						if (LocalVars.Contains(Base))
							ACBaseVar.Add(ACInst->GetId().value(), Base);
					}
				}
			}

			// Find entry label and compute GEN sets
			if (BlockLabels.IsEmpty())
				continue;
			SpvId FuncEntryLabel = BlockLabels[0];
			TMap<SpvId, TSet<SpvId>> Gen;

			for (SpvId Label : BlockLabels)
			{
				Gen.Add(Label, {});
			}

			// Compute GEN sets: direct OpStore to local var, or OpVariable with initializer
			for (auto& [Label, GenSet] : Gen)
			{
				const auto& BB = BBs.at(Label);
				if (BB.EndIdx < 0) continue;
				for (int32 i = BB.StartIdx; i <= BB.EndIdx; i++)
				{
					if (auto* StoreInst = dynamic_cast<SpvOpStore*>((*Insts)[i].Get()))
					{
						if (LocalVars.Contains(StoreInst->GetPointer()))
							GenSet.Add(StoreInst->GetPointer());
					}
					else if (auto* VarInst = dynamic_cast<SpvOpVariable*>((*Insts)[i].Get()))
					{
						SpvId VarId = VarInst->GetId().value();
						if (VarInst->GetInitializer().has_value() && LocalVars.Contains(VarId))
							GenSet.Add(VarId);
					}
				}
			}

			// ---- Forward definite-assignment dataflow ----
			// Domain: TSet<SpvId> per block (set of definitely-assigned local vars)
			// Meet: intersection (∩)  — var is assigned only if assigned on ALL predecessors
			// Transfer: DefOut[B] = DefIn[B] ∪ Gen[B]
			// Init: DefOut[entry] = Gen[entry], DefOut[others] = LocalVars (optimistic)
			TMap<SpvId, TSet<SpvId>> DefOut;
			for (auto& [Label, GenSet] : Gen)
			{
				if (Label == FuncEntryLabel)
					DefOut.Add(Label, GenSet);
				else
					DefOut.Add(Label, LocalVars);
			}

			bool Changed = true;
			while (Changed)
			{
				Changed = false;
				for (auto& [Label, GenSet] : Gen)
				{
					if (Label == FuncEntryLabel)
						continue;

					TSet<SpvId> NewDefIn;
					const auto& Preds = BBs.at(Label).Predecessors;
					if (Preds.Num() == 0)
					{
						NewDefIn = LocalVars; // Unreachable block
					}
					else
					{
						NewDefIn = DefOut[Preds[0]];
						for (int32 p = 1; p < Preds.Num(); p++)
						{
							TSet<SpvId> Intersected;
							for (SpvId Id : NewDefIn)
							{
								if (DefOut[Preds[p]].Contains(Id))
									Intersected.Add(Id);
							}
							NewDefIn = MoveTemp(Intersected);
						}
					}

					TSet<SpvId> NewDefOut = NewDefIn;
					NewDefOut.Append(GenSet);

					// Lattice is monotonically descending; size decrease ⟹ change
					if (NewDefOut.Num() < DefOut[Label].Num())
					{
						DefOut[Label] = MoveTemp(NewDefOut);
						Changed = true;
					}
				}
			}

			// Compute final DefIn per block
			TMap<SpvId, TSet<SpvId>> DefIn;
			for (auto& [Label, GenSet] : Gen)
			{
				if (Label == FuncEntryLabel)
				{
					DefIn.Add(Label, TSet<SpvId>{});
				}
				else
				{
					const auto& Preds = BBs.at(Label).Predecessors;
					if (Preds.Num() == 0)
					{
						DefIn.Add(Label, LocalVars);
					}
					else
					{
						TSet<SpvId> In = DefOut[Preds[0]];
						for (int32 p = 1; p < Preds.Num(); p++)
						{
							TSet<SpvId> Intersected;
							for (SpvId Id : In)
							{
								if (DefOut[Preds[p]].Contains(Id))
									Intersected.Add(Id);
							}
							In = MoveTemp(Intersected);
						}
						DefIn.Add(Label, MoveTemp(In));
					}
				}
			}

			// ---- Check all loads: find vars with at least one uninitialized read ----
			TSet<SpvId> UnsafeVars;
			for (auto& [Label, GenSet] : Gen)
			{
				const auto& BB = BBs.at(Label);
				if (BB.EndIdx < 0) continue;
				TSet<SpvId> Assigned = DefIn[Label];

				for (int32 i = BB.StartIdx; i <= BB.EndIdx; i++)
				{
					if (auto* VarInst = dynamic_cast<SpvOpVariable*>((*Insts)[i].Get()))
					{
						SpvId VarId = VarInst->GetId().value();
						if (VarInst->GetInitializer().has_value() && LocalVars.Contains(VarId))
							Assigned.Add(VarId);
					}
					else if (auto* StoreInst = dynamic_cast<SpvOpStore*>((*Insts)[i].Get()))
					{
						if (LocalVars.Contains(StoreInst->GetPointer()))
							Assigned.Add(StoreInst->GetPointer());
					}
					else if (auto* LoadInst = dynamic_cast<SpvOpLoad*>((*Insts)[i].Get()))
					{
						SpvId Pointer = LoadInst->GetPointer();
						SpvId BaseVar = Pointer;
						if (SpvId* Base = ACBaseVar.Find(Pointer))
							BaseVar = *Base;
						if (LocalVars.Contains(BaseVar) && !Assigned.Contains(BaseVar))
							UnsafeVars.Add(BaseVar);
					}
				}
			}

			// All local vars not in UnsafeVars are statically initialized
			for (SpvId VarId : LocalVars)
			{
				if (!UnsafeVars.Contains(VarId))
					Result.Add(VarId);
			}
		}

		return Result;
	}

	SpvId PatchDebuggerBuffer(SpvPatcher& Patcher)
	{
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UVec4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 4));
		SpvId RunTimeArrayType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeRuntimeArray>(UVec4Type));
		SpvId DebuggerBufferType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeStruct>(TArray<SpvId>{RunTimeArrayType}));
		SpvId DebuggerBufferPointerType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, DebuggerBufferType));

		int ArrayStride = 16;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(RunTimeArrayType, SpvDecorationKind::ArrayStride, TArray<uint8>{ (uint8*)&ArrayStride, sizeof(int) }));
		int MemberOffset = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpMemberDecorate>(DebuggerBufferType, 0, SpvDecorationKind::Offset, TArray<uint8>{ (uint8*)&MemberOffset, sizeof(int) }));
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBufferType, SpvDecorationKind::BufferBlock));

		SpvId DebuggerBuffer = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(DebuggerBufferPointerType, SpvStorageClass::Uniform);
			VarOp->SetId(DebuggerBuffer);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerBuffer, "_DebuggerBuffer_"));
		}
		int SetNumber = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBuffer, SpvDecorationKind::DescriptorSet, TArray<uint8>{ (uint8*)&SetNumber, sizeof(int) }));
		int BindingNumber = DebuggerBufferBindingSlot;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBuffer, SpvDecorationKind::Binding, TArray<uint8>{ (uint8*)&BindingNumber, sizeof(int) }));

		return DebuggerBuffer;
	}

	// Creates a uniform buffer containing: uvec2 PixelCoord
	SpvId PatchDebuggerParams(SpvPatcher& Patcher)
	{
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UInt2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 2));
		
		// Create struct type with uvec2 member for PixelCoord
		SpvId DebuggerParamsType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeStruct>(TArray<SpvId>{UInt2Type}));
		SpvId DebuggerParamsPointerType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, DebuggerParamsType));

		// Decorations
		int MemberOffset = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpMemberDecorate>(DebuggerParamsType, 0, SpvDecorationKind::Offset, TArray<uint8>{ (uint8*)&MemberOffset, sizeof(int) }));
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerParamsType, SpvDecorationKind::Block));

		SpvId DebuggerParams = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(DebuggerParamsPointerType, SpvStorageClass::Uniform);
			VarOp->SetId(DebuggerParams);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerParams, "_DebuggerParams_"));
		}
		int SetNumber = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerParams, SpvDecorationKind::DescriptorSet, TArray<uint8>{ (uint8*)&SetNumber, sizeof(int) }));
		int BindingNumber = DebuggerParamsBindingSlot;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerParams, SpvDecorationKind::Binding, TArray<uint8>{ (uint8*)&BindingNumber, sizeof(int) }));

		return DebuggerParams;
	}

	void SpvDebuggerVisitor::Visit(const SpvDebugDeclare* Inst)
	{
		SpvVariableDesc* VarDesc = &Context.VariableDescs[Inst->GetVarDesc()];
		Context.VariableDescMap.emplace(Inst->GetVariable(),VarDesc);
	}

	void SpvDebuggerVisitor::Visit(const SpvDebugValue* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvDebugFunctionDefinition* Inst)
	{
		SpvFunctionDesc* FuncDesc = static_cast<SpvFunctionDesc*>(Context.LexicalScopes[Inst->GetFunction()].Get());
		FuncDesc->DefinitionType = static_cast<SpvFunctionType*>(Context.Types[CurFunc->Type].Get());
	}

	void SpvDebuggerVisitor::Visit(const SpvDebugLine* Inst)
	{
		CurLine = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLineStart()].Storage).Value.GetData();
		CurSource = Inst->GetSource();
	}

	void SpvDebuggerVisitor::Visit(const SpvDebugScope* Inst)
	{
		CurScope = Context.LexicalScopes[Inst->GetScope()].Get();
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFunction* Inst)
	{
		CurFunc = &Context.Funcs[Inst->GetId().value()];
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFunctionCall* Inst)
	{
		if (CurLine && CurBlock)
		{
			CurBlock->ValidLines.AddUnique(CurLine);
		}

		SpvId ResultId = Inst->GetId().value();
		Context.FuncCalls.emplace(ResultId, SpvFuncCall{Inst->GetFunction(), Inst->GetArguments() });
		
		TArray<TUniquePtr<SpvInstruction>> AppendCallInsts;
		uint32 PackedHeaderValue = PackDebugHeader(SpvDebuggerStateType::FuncCall, CurSource.GetValue(), (uint32)CurLine);
		if (CurScope) { Context.HeaderToScope.Add(PackedHeaderValue, CurScope->GetId()); }
		SpvId PackedHeader = Patcher.FindOrAddConstant(PackedHeaderValue);
		SpvId CallId = Patcher.FindOrAddConstant(ResultId.GetValue());
		{
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendCallFuncId, TArray<SpvId>{ PackedHeader, CallId});
			FuncCallOp->SetId(Patcher.NewId());
			AppendCallInsts.Add(MoveTemp(FuncCallOp));
		}
		PostAppendCall(AppendCallInsts, PackedHeader, CallId);
		Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AppendCallInsts));

		//Insert PostCallReturn instructions after the original OpFunctionCall
		TArray<TUniquePtr<SpvInstruction>> PostReturnInsts;
		PostCallReturn(PostReturnInsts, CallId);
		if (PostReturnInsts.Num() > 0)
		{
			Patcher.AddInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(PostReturnInsts));
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFunctionParameter* Inst)
	{
		SpvId ResultId = Inst->GetId().value();

		SpvType* ParamType = Context.Types[Inst->GetResultType()].Get();
		if (ParamType->GetKind() == SpvTypeKind::Pointer)
		{
			SpvPointerType* PointerType = static_cast<SpvPointerType*>(ParamType);
			SpvVariable Var = { {ResultId, PointerType->PointeeType}, SpvStorageClass::Function, PointerType};

			TArray<uint8> Value;
			Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
			Var.Storage = SpvObject::Internal{ MoveTemp(Value) };

			Context.LocalVariables.emplace(ResultId, MoveTemp(Var));
			SpvPointer Pointer{
				.Id = ResultId,
				.Var = &Context.LocalVariables[ResultId],
				.Type = PointerType
			};
			Context.LocalPointers.emplace(ResultId, MoveTemp(Pointer));
		}
		else
		{
			SpvVariable Var = { {ResultId, ParamType}, SpvStorageClass::Function};

			TArray<uint8> Value;
			Value.SetNumZeroed(GetTypeByteSize(ParamType));
			Var.Storage = SpvObject::Internal{ MoveTemp(Value) };

			Context.LocalVariables.emplace(ResultId, MoveTemp(Var));
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpVariable* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		
		SpvStorageClass StorageClass = Inst->GetStorageClass();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = {{ResultId, PointerType->PointeeType}, StorageClass, PointerType };

		TArray<uint8> Value;
		Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
		Var.Storage = SpvObject::Internal{MoveTemp(Value)};
		
		Context.LocalVariables.emplace(ResultId, MoveTemp(Var));
		SpvPointer Pointer{
			.Id = ResultId,
			.Var = &Context.LocalVariables[ResultId],
			.Type = PointerType
		};
		Context.LocalPointers.emplace(ResultId, MoveTemp(Pointer));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpPhi* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpLabel* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		Context.BBs.try_emplace(ResultId, SpvBasicBlock{});
		CurBlock = &Context.BBs[ResultId];
	}

	void SpvDebuggerVisitor::Visit(const SpvOpLoad* Inst)
	{
		if (EnableUbsan)
		{
			SpvPointer* Pointer = Context.FindPointer(Inst->GetPointer());
			// Only check initialization for user-declared variables
			if (Pointer && Context.VariableDescMap.contains(Pointer->Var->Id))
			{
				if (StaticallyInitializedVars.Contains(Pointer->Var->Id))
				{
					// Statically initialized: skip AppendAccess entirely when all indexes are constants.
					// Dynamic indexes still need AppendAccess for bounds checking on the CPU side.
					bool HasDynamicIndex = false;
					for (SpvId Index : Pointer->Indexes)
					{
						if (!Context.Constants.contains(Index))
						{
							HasDynamicIndex = true;
							break;
						}
					}
					if (HasDynamicIndex)
					{
						uint32 PackedHeaderValue = PackDebugHeader(SpvDebuggerStateType::Access, CurSource.GetValue(), (uint32)CurLine);
						uint64 Key = ((uint64)PackedHeaderValue << 32) | Pointer->Var->Id.GetValue();
						Context.AccessCheckMasks.FindOrAdd(Key, 0) |= 0xFFFFFFFFu;

						AppendAccess([&] {return Inst->GetWordOffset().value(); }, Pointer);
					}
				}
				else
				{
					// Not statically initialized: full init-check path
					uint32 CheckMask = 0xFFFFFFFFu;
					if (uint32* Mask = LoadUsedInitUnits.Find(Inst->GetId().value()))
					{
						CheckMask = *Mask;
					}
					if (CheckMask != 0)
					{
						uint32 PackedHeaderValue = PackDebugHeader(SpvDebuggerStateType::Access, CurSource.GetValue(), (uint32)CurLine);
						uint64 Key = ((uint64)PackedHeaderValue << 32) | Pointer->Var->Id.GetValue();
						Context.AccessCheckMasks.FindOrAdd(Key, 0) |= CheckMask;

						AppendAccess([&] {return Inst->GetWordOffset().value(); }, Pointer);
					}
				}
			}
		}

	}

	void SpvDebuggerVisitor::Visit(const SpvOpStore* Inst)
	{
		SpvPointer* Pointer = Context.FindPointer(Inst->GetPointer());
		if (!Pointer)
		{
			return;
		}
		if (Context.Constants.contains(Inst->GetObject()))
		{
			uint32 PH = PackDebugHeader(SpvDebuggerStateType::VarChange, CurSource.GetValue(), (uint32)CurLine);
			Context.ConstInitVarChanges.Add(((uint64)PH << 32) | Pointer->Var->Id.GetValue());
		}

		// Detect vector swizzle write (read-modify-write via OpVectorShuffle).
		// Instead of recording the full vector store, emit per-component AppendVar calls
		// for only the actually-written components, so CPU-side gets precise byte ranges.
		SpvType* PointeeType = Pointer->Type->PointeeType;
		if (PointeeType->GetKind() == SpvTypeKind::Vector)
		{
			SpvId ObjectId = Inst->GetObject();
			int32 DefIdx = GetInstIndex(Insts, ObjectId);
			if (DefIdx != INDEX_NONE)
			{
				if (auto* Shuffle = dynamic_cast<SpvOpVectorShuffle*>((*Insts)[DefIdx].Get()))
				{
					SpvId Vec1Id = Shuffle->GetVector1();
					SpvVectorType* VecType = static_cast<SpvVectorType*>(PointeeType);
					uint32 Vec1Count = VecType->ElementCount;

					auto IsLoadOfSamePtr = [&](SpvId VecId) -> bool {
						int32 LoadIdx = GetInstIndex(Insts, VecId);
						if (LoadIdx == INDEX_NONE) return false;
						auto* Load = dynamic_cast<SpvOpLoad*>((*Insts)[LoadIdx].Get());
						return Load && Load->GetPointer() == Inst->GetPointer();
					};

					if (IsLoadOfSamePtr(Vec1Id))
					{
						const TArray<uint32>& Comps = Shuffle->GetComponents();
						uint32 WriteMask = 0;
						for (int32 i = 0; i < Comps.Num(); i++)
						{
							if (Comps[i] >= Vec1Count)
								WriteMask |= (1u << i);
						}

						if (WriteMask != 0 && WriteMask != ((1u << Comps.Num()) - 1))
						{
							// Emit per-component AppendVar for each written component.
							SpvScalarType* ElementType = VecType->ElementType;
							SpvPointerType* VecPtrType = Pointer->Type;
							SpvId ScalarPtrTypeId = Patcher.FindOrAddType(
								MakeUnique<SpvOpTypePointer>(VecPtrType->StorageClass, ElementType->GetId()));
							SpvPointerType* ScalarPtrType = static_cast<SpvPointerType*>(
								Context.Types[ScalarPtrTypeId].Get());

							// Build OpAccessChain instructions for all written components.
							TArray<TUniquePtr<SpvInstruction>> AccessChainInsts;
							TArray<SpvId> CompPtrIds;
							for (int32 i = 0; i < Comps.Num(); i++)
							{
								if (!(WriteMask & (1u << i)))
									continue;

								SpvId CompIndex = Patcher.FindOrAddConstant((uint32)i);
								SpvId CompPtrId = Patcher.NewId();
								auto ChainOp = MakeUnique<SpvOpAccessChain>(ScalarPtrTypeId, Pointer->Id, TArray<SpvId>{CompIndex});
								ChainOp->SetId(CompPtrId);
								AccessChainInsts.Add(MoveTemp(ChainOp));

								// Register as local pointer so AppendVar can resolve type/indexes.
								TArray<SpvId> CompIndexes = Pointer->Indexes;
								CompIndexes.Add(CompIndex);
								Context.LocalPointers.emplace(CompPtrId, SpvPointer{
									.Id = CompPtrId,
									.Var = Pointer->Var,
									.Indexes = MoveTemp(CompIndexes),
									.Type = ScalarPtrType
								});
								CompPtrIds.Add(CompPtrId);
							}

							// Call AppendVar for each component (reverse ordered by Patcher,
							// but all OpAccessChains will be emitted after, placing them before all loads).
							auto OffsetEval = [&] { return Inst->GetWordOffset().value() + Inst->GetWordLen().value(); };
							for (SpvId CompPtrId : CompPtrIds)
							{
								SpvPointer* CompPointer = &Context.LocalPointers[CompPtrId];
								if (SpvOpFunctionCall* FuncCall = AppendVar(OffsetEval, CompPointer))
								{
									AppendVarCallToStore.Add(FuncCall, Inst);
								}
							}

							// Emit all OpAccessChains — added last so they appear first in binary.
							Patcher.AddInstructions(OffsetEval(), MoveTemp(AccessChainInsts));
							return;
						}
					}
				}
			}
		}

		if (SpvOpFunctionCall* FuncCall = AppendVar([&] {return Inst->GetWordOffset().value() + Inst->GetWordLen().value(); }, Pointer))
		{
			AppendVarCallToStore.Add(FuncCall, Inst);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpAccessChain* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvPointer* BasePointer = Context.FindPointer(Inst->GetBasePointer());
		TArray<SpvId> Indexes = BasePointer->Indexes;
		Indexes.Append(Inst->GetIndexes());

		SpvPointer Pointer{
			.Id = ResultId,
			.Var = BasePointer->Var,
			.Indexes = MoveTemp(Indexes),
			.Type = PointerType
		};
		Context.LocalPointers.emplace(ResultId, MoveTemp(Pointer));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpSwitch* Inst)
	{
		AppendTag([&] { return (*Insts)[InstIndex - 1]->GetWordOffset().value(); }, SpvDebuggerStateType::Condition);
	}

	void SpvDebuggerVisitor::Visit(const SpvOpConvertFToU* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId FloatTypeId = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
			if (SpvVectorType* VecType = dynamic_cast<SpvVectorType*>(ResultType))
			{
				FloatTypeId = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatTypeId, VecType->ElementCount));
			}
			SpvType* SrcType = Context.Types[FloatTypeId].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, SrcType, { Inst->GetFloatValue() }, SpvDebuggerStateType::ConvertF);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpConvertFToS* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId FloatTypeId = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
			if (SpvVectorType* VecType = dynamic_cast<SpvVectorType*>(ResultType))
			{
				FloatTypeId = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatTypeId, VecType->ElementCount));
			}
			SpvType* SrcType = Context.Types[FloatTypeId].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, SrcType, { Inst->GetFloatValue() }, SpvDebuggerStateType::ConvertF);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpUDiv* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetOperand2() }, SpvDebuggerStateType::Div);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpSDiv* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetOperand2() }, SpvDebuggerStateType::Div);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFDiv* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetOperand2() }, SpvDebuggerStateType::Div);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpUMod* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetOperand1(), Inst->GetOperand2() }, SpvDebuggerStateType::Remainder);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpSRem* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetOperand1(), Inst->GetOperand2() }, SpvDebuggerStateType::Remainder);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFRem* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetOperand1(), Inst->GetOperand2() }, SpvDebuggerStateType::Remainder);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpBranch* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpBranchConditional* Inst)
	{
		SpvInstruction* PrevInst = (*Insts)[InstIndex - 1].Get();
		if (dynamic_cast<SpvOpLoopMerge*>(PrevInst) || dynamic_cast<SpvOpSelectionMerge*>(PrevInst))
		{
			AppendTag([&] { return PrevInst->GetWordOffset().value(); }, SpvDebuggerStateType::Condition);
		}
		else
		{
			AppendTag([&] { return Inst->GetWordOffset().value(); }, SpvDebuggerStateType::Condition);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpReturn* Inst)
	{
		AppendTag([&] { return Inst->GetWordOffset().value(); }, SpvDebuggerStateType::Return);
	}

	void SpvDebuggerVisitor::Visit(const SpvOpReturnValue* Inst)
	{
		SpvType* CurReturnType = Context.Types[CurFunc->ReturnType].Get();
		AppendValue([&] { return Inst->GetWordOffset().value(); }, CurReturnType, Inst->GetValue(), SpvDebuggerStateType::ReturnValue);
		AppendTag([&] { return Inst->GetWordOffset().value(); }, SpvDebuggerStateType::Return);
	}

	void SpvDebuggerVisitor::Visit(const SpvPow* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetX(), Inst->GetY() }, SpvDebuggerStateType::Pow);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvFClamp* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType,{ Inst->GetMinVal(), Inst->GetMaxVal()}, SpvDebuggerStateType::Clamp);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvUClamp* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetMinVal(), Inst->GetMaxVal() }, SpvDebuggerStateType::Clamp);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvSClamp* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetMinVal(), Inst->GetMaxVal() }, SpvDebuggerStateType::Clamp);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvSmoothStep* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetEdge0(), Inst->GetEdge1()}, SpvDebuggerStateType::SmoothStep);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvNormalize* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetX() }, SpvDebuggerStateType::Normalize);
		}

	}

	void SpvDebuggerVisitor::Visit(const SpvLog* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetX() }, SpvDebuggerStateType::Log);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvLog2* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetX() }, SpvDebuggerStateType::Log);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvAsin* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetX() }, SpvDebuggerStateType::Asin);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvAcos* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetX() }, SpvDebuggerStateType::Acos);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvSqrt* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetX() }, SpvDebuggerStateType::Sqrt);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvInverseSqrt* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetX() }, SpvDebuggerStateType::InverseSqrt);
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvAtan2* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			AppendMath([&] { return Inst->GetWordOffset().value(); }, ResultType, ResultType, { Inst->GetY(), Inst->GetX() }, SpvDebuggerStateType::Atan2);
		}
	}

	void SpvDebuggerVisitor::PatchToDebugger(SpvId InValueId, SpvId InTypeId, TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		TArray<SpvId> UIntValues;
		FlattenToUInts(InValueId, InTypeId, InstList, UIntValues);
		BatchStoreToDebugBuffer(UIntValues, InstList);
	}

	void SpvDebuggerVisitor::FlattenToUInts(SpvId InValueId, SpvId InTypeId, TArray<TUniquePtr<SpvInstruction>>& InstList, TArray<SpvId>& OutUIntValues)
	{
		SpvType* Type = Context.Types.at(InTypeId).Get();
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		if (InTypeId == UIntType)
		{
			OutUIntValues.Add(InValueId);
		}
		else if (Type->IsScalar())
		{
			if (Type->GetKind() == SpvTypeKind::Bool)
			{
				SpvId SelectValue = Patcher.NewId();
				auto SelectOp = MakeUnique<SpvOpSelect>(UIntType, InValueId, Patcher.FindOrAddConstant(1u), Patcher.FindOrAddConstant(0u));
				SelectOp->SetId(SelectValue);
				InstList.Add(MoveTemp(SelectOp));
				OutUIntValues.Add(SelectValue);
			}
			else if (SpvIntegerType* IntegerType = dynamic_cast<SpvIntegerType*>(Type); IntegerType && IntegerType->GetWidth() == 64)
			{
				SpvId UInt2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 2));
				SpvId UInt64Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(64, 0));
				SpvId CastInput = InValueId;
				if (IntegerType->IsSigend())
				{
					CastInput = Patcher.NewId();
					auto BitCastOp = MakeUnique<SpvOpBitcast>(UInt64Type, InValueId);
					BitCastOp->SetId(CastInput);
					InstList.Add(MoveTemp(BitCastOp));
				}
				SpvId BitCastValue = Patcher.NewId();
				auto BitCastOp = MakeUnique<SpvOpBitcast>(UInt2Type, CastInput);
				BitCastOp->SetId(BitCastValue);
				InstList.Add(MoveTemp(BitCastOp));
				FlattenToUInts(BitCastValue, UInt2Type, InstList, OutUIntValues);
			}
			else
			{
				SpvId BitCastValue = Patcher.NewId();
				auto BitCastOp = MakeUnique<SpvOpBitcast>(UIntType, InValueId);
				BitCastOp->SetId(BitCastValue);
				InstList.Add(MoveTemp(BitCastOp));
				OutUIntValues.Add(BitCastValue);
			}
		}
		else if (Type->GetKind() == SpvTypeKind::Vector)
		{
			SpvVectorType* VectorType = static_cast<SpvVectorType*>(Type);
			for (uint32 Index = 0; Index < VectorType->ElementCount; Index++)
			{
				SpvId ElementTypeId = VectorType->ElementType->GetId();
				SpvId ExtractedValue = Patcher.NewId();
				auto ExtractOp = MakeUnique<SpvOpCompositeExtract>(ElementTypeId, InValueId, TArray<uint32>{Index});
				ExtractOp->SetId(ExtractedValue);
				InstList.Add(MoveTemp(ExtractOp));
				FlattenToUInts(ExtractedValue, ElementTypeId, InstList, OutUIntValues);
			}
		}
		else if (Type->GetKind() == SpvTypeKind::Struct)
		{
			SpvStructType* StructType = static_cast<SpvStructType*>(Type);
			for (uint32 Index = 0; Index < (uint32)StructType->MemberTypes.Num(); Index++)
			{
				SpvId MemberTypeId = StructType->MemberTypes[Index]->GetId();
				SpvId ExtractedValue = Patcher.NewId();
				auto ExtractOp = MakeUnique<SpvOpCompositeExtract>(MemberTypeId, InValueId, TArray<uint32>{Index});
				ExtractOp->SetId(ExtractedValue);
				InstList.Add(MoveTemp(ExtractOp));
				FlattenToUInts(ExtractedValue, MemberTypeId, InstList, OutUIntValues);
			}
		}
		else if (Type->GetKind() == SpvTypeKind::Array)
		{
			SpvArrayType* ArrayType = static_cast<SpvArrayType*>(Type);
			for (uint32 Index = 0; Index < ArrayType->Length; Index++)
			{
				SpvId ElementTypeId = ArrayType->ElementType->GetId();
				SpvId ExtractedValue = Patcher.NewId();
				auto ExtractOp = MakeUnique<SpvOpCompositeExtract>(ElementTypeId, InValueId, TArray<uint32>{Index});
				ExtractOp->SetId(ExtractedValue);
				InstList.Add(MoveTemp(ExtractOp));
				FlattenToUInts(ExtractedValue, ElementTypeId, InstList, OutUIntValues);
			}
		}
		else if (Type->GetKind() == SpvTypeKind::Matrix)
		{
			SpvMatrixType* MatrixType = static_cast<SpvMatrixType*>(Type);
			for (uint32 Index = 0; Index < MatrixType->ElementCount; Index++)
			{
				SpvId ElementTypeId = MatrixType->ElementType->GetId();
				SpvId ExtractedValue = Patcher.NewId();
				auto ExtractOp = MakeUnique<SpvOpCompositeExtract>(ElementTypeId, InValueId, TArray<uint32>{Index});
				ExtractOp->SetId(ExtractedValue);
				InstList.Add(MoveTemp(ExtractOp));
				FlattenToUInts(ExtractedValue, ElementTypeId, InstList, OutUIntValues);
			}
		}
	}

	void SpvDebuggerVisitor::BatchStoreToDebugBuffer(const TArray<SpvId>& UIntValues, TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		if (UIntValues.IsEmpty())
		{
			return;
		}

		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UVec4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 4));
		SpvId UVec4PointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UVec4Type));
		SpvId Zero = Patcher.FindOrAddConstant(0u);

		// Load offset once
		SpvId LoadedDebuggerOffset = Patcher.NewId();
		{
			auto Op = MakeUnique<SpvOpLoad>(UIntType, DebuggerOffset);
			Op->SetId(LoadedDebuggerOffset);
			InstList.Add(MoveTemp(Op));
		}

		// Base index = offset / 16 (since each element is uvec4 = 16 bytes)
		SpvId BaseIndex = Patcher.NewId();
		{
			auto ShiftOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant(4u));
			ShiftOp->SetId(BaseIndex);
			InstList.Add(MoveTemp(ShiftOp));
		}

		int32 NumValues = UIntValues.Num();
		int32 NumFullVec4s = NumValues / 4;
		int32 Remainder = NumValues % 4;

		// Store full uvec4 groups
		for (int32 i = 0; i < NumFullVec4s; i++)
		{
			// Construct uvec4
			SpvId Vec4Value = Patcher.NewId();
			{
				auto ConstructOp = MakeUnique<SpvOpCompositeConstruct>(UVec4Type, TArray<SpvId>{
					UIntValues[i * 4 + 0], UIntValues[i * 4 + 1], UIntValues[i * 4 + 2], UIntValues[i * 4 + 3]
				});
				ConstructOp->SetId(Vec4Value);
				InstList.Add(MoveTemp(ConstructOp));
			}

			// AccessChain to buffer[0][baseIndex + i]
			SpvId StoreIndex;
			if (i == 0)
			{
				StoreIndex = BaseIndex;
			}
			else
			{
				StoreIndex = Patcher.NewId();
				auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, BaseIndex, Patcher.FindOrAddConstant((uint32)i));
				AddOp->SetId(StoreIndex);
				InstList.Add(MoveTemp(AddOp));
			}

			SpvId StoragePtr = Patcher.NewId();
			{
				auto ChainOp = MakeUnique<SpvOpAccessChain>(UVec4PointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, StoreIndex});
				ChainOp->SetId(StoragePtr);
				InstList.Add(MoveTemp(ChainOp));
			}

			InstList.Add(MakeUnique<SpvOpStore>(StoragePtr, Vec4Value));
		}

		// Store remaining values (pad with 0)
		if (Remainder > 0)
		{
			TArray<SpvId> Components;
			for (int32 i = 0; i < Remainder; i++)
			{
				Components.Add(UIntValues[NumFullVec4s * 4 + i]);
			}
			for (int32 i = Remainder; i < 4; i++)
			{
				Components.Add(Zero);
			}

			SpvId Vec4Value = Patcher.NewId();
			{
				auto ConstructOp = MakeUnique<SpvOpCompositeConstruct>(UVec4Type, Components);
				ConstructOp->SetId(Vec4Value);
				InstList.Add(MoveTemp(ConstructOp));
			}

			SpvId StoreIndex;
			if (NumFullVec4s == 0)
			{
				StoreIndex = BaseIndex;
			}
			else
			{
				StoreIndex = Patcher.NewId();
				auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, BaseIndex, Patcher.FindOrAddConstant((uint32)NumFullVec4s));
				AddOp->SetId(StoreIndex);
				InstList.Add(MoveTemp(AddOp));
			}

			SpvId StoragePtr = Patcher.NewId();
			{
				auto ChainOp = MakeUnique<SpvOpAccessChain>(UVec4PointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, StoreIndex});
				ChainOp->SetId(StoragePtr);
				InstList.Add(MoveTemp(ChainOp));
			}

			InstList.Add(MakeUnique<SpvOpStore>(StoragePtr, Vec4Value));
		}

		// Update offset: advance by ceil(NumValues/4)*16 bytes (uvec4-aligned)
		int32 TotalVec4s = NumFullVec4s + (Remainder > 0 ? 1 : 0);
		SpvId NewDebuggerOffset = Patcher.NewId();
		{
			auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant((uint32)(TotalVec4s * 16)));
			AddOp->SetId(NewDebuggerOffset);
			InstList.Add(MoveTemp(AddOp));
		}
		InstList.Add(MakeUnique<SpvOpStore>(DebuggerOffset, NewDebuggerOffset));
	}

	void SpvDebuggerVisitor::PatchAppendAccessFunc(int32 IndexNum)
	{
		if (AppendAccessFuncIds.Contains(IndexNum))
		{
			return;
		}

		SpvId AppendAccessFuncId = Patcher.NewId();
		FString FuncName = FString::Printf(TEXT("_AppendAccess_%d_"), IndexNum);
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendAccessFuncId, FuncName));
		TArray<TUniquePtr<SpvInstruction>> AppendAccessFuncInsts;
		{
			SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> ParamterTypes;
			ParamterTypes.Add(UIntType); // PackedHeader
			ParamterTypes.Add(UIntType); // VarId
			for (int32 i = 0; i < IndexNum; i++)
			{
				ParamterTypes.Add(IntType);
			}

			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, ParamterTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendAccessFuncId);
			AppendAccessFuncInsts.Add(MoveTemp(FuncOp));

			SpvId PackedHeaderParam = Patcher.NewId();
			auto PackedHeaderParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			PackedHeaderParamOp->SetId(PackedHeaderParam);
			AppendAccessFuncInsts.Add(MoveTemp(PackedHeaderParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(PackedHeaderParam, "_DebuggerPackedHeader_"));

			SpvId VarIdParam = Patcher.NewId();
			auto VarIdParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			VarIdParamOp->SetId(VarIdParam);
			AppendAccessFuncInsts.Add(MoveTemp(VarIdParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(VarIdParam, "_DebuggerVarId_"));

			TArray<SpvId> IndexParams;
			for (int32 i = 0; i < IndexNum; i++)
			{
				SpvId IndexParam = Patcher.NewId();
				auto IndexParamOp = MakeUnique<SpvOpFunctionParameter>(IntType);
				IndexParamOp->SetId(IndexParam);
				AppendAccessFuncInsts.Add(MoveTemp(IndexParamOp));
				Patcher.AddDebugName(MakeUnique<SpvOpName>(IndexParam, FString::Printf(TEXT("_DebuggerIndex%d_"), i)));
				IndexParams.Add(IndexParam);
			}

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendAccessFuncInsts.Add(MoveTemp(LabelOp));

			AppendAccessFuncIds.Add(IndexNum, AppendAccessFuncId);
			if (PatchActiveCondition(AppendAccessFuncInsts))
			{
				TArray<SpvId> AllValues;
				FlattenToUInts(PackedHeaderParam, UIntType, AppendAccessFuncInsts, AllValues);
				FlattenToUInts(VarIdParam, UIntType, AppendAccessFuncInsts, AllValues);
				FlattenToUInts(Patcher.FindOrAddConstant((uint32)IndexNum), UIntType, AppendAccessFuncInsts, AllValues);
				for (int32 i = 0; i < IndexNum; i++)
				{
					FlattenToUInts(IndexParams[i], IntType, AppendAccessFuncInsts, AllValues);
				}
				BatchStoreToDebugBuffer(AllValues, AppendAccessFuncInsts);
			}

			AppendAccessFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendAccessFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendAccessFuncInsts));
	}

	void SpvDebuggerVisitor::PatchAppendMathFunc(SpvType* ResultType, SpvType* OperandType, int32 OperandNum)
	{
		if (AppendMathFuncIds.Contains({ ResultType, OperandType, OperandNum }))
		{
			return;
		}

		SpvId AppendMathFuncId = Patcher.NewId();
		FString FuncName = FString::Printf(TEXT("_AppendMath_%d_%d_"), ResultType->GetId().GetValue(), OperandType->GetId().GetValue(), OperandNum);
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendMathFuncId, FuncName));
		TArray<TUniquePtr<SpvInstruction>> AppendMathFuncInsts;
		{
			SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> ParamterTypes;
			ParamterTypes.Add(UIntType); // PackedHeader
			ParamterTypes.Add(UIntType); // ResultTypeId
			for (int32 i = 0; i < OperandNum; i++)
			{
				ParamterTypes.Add(OperandType->GetId());
			}

			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, ParamterTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendMathFuncId);
			AppendMathFuncInsts.Add(MoveTemp(FuncOp));

			SpvId PackedHeaderParam = Patcher.NewId();
			auto PackedHeaderParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			PackedHeaderParamOp->SetId(PackedHeaderParam);
			AppendMathFuncInsts.Add(MoveTemp(PackedHeaderParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(PackedHeaderParam, "_DebuggerPackedHeader_"));

			SpvId ResultTypeIdParam = Patcher.NewId();
			auto ResultTypeIdParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			ResultTypeIdParamOp->SetId(ResultTypeIdParam);
			AppendMathFuncInsts.Add(MoveTemp(ResultTypeIdParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(ResultTypeIdParam, "_DebuggerResultTypeId_"));

			TArray<SpvId> OperandParams;
			for (int32 i = 0; i < OperandNum; i++)
			{
				SpvId OperandParam = Patcher.NewId();
				auto OperandParamOp = MakeUnique<SpvOpFunctionParameter>(OperandType->GetId());
				OperandParamOp->SetId(OperandParam);
				AppendMathFuncInsts.Add(MoveTemp(OperandParamOp));
				Patcher.AddDebugName(MakeUnique<SpvOpName>(OperandParam, FString::Printf(TEXT("_DebuggerOperand%d_"), i)));
				OperandParams.Add(OperandParam);
			}

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendMathFuncInsts.Add(MoveTemp(LabelOp));

			AppendMathFuncIds.Add({ ResultType, OperandType, OperandNum }, AppendMathFuncId);
			if (PatchActiveCondition(AppendMathFuncInsts))
			{
				TArray<SpvId> AllValues;
				FlattenToUInts(PackedHeaderParam, UIntType, AppendMathFuncInsts, AllValues);
				FlattenToUInts(ResultTypeIdParam, UIntType, AppendMathFuncInsts, AllValues);
				for (int32 i = 0; i < OperandNum; i++)
				{
					FlattenToUInts(OperandParams[i], OperandType->GetId(), AppendMathFuncInsts, AllValues);
				}
				BatchStoreToDebugBuffer(AllValues, AppendMathFuncInsts);
			}

			AppendMathFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendMathFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendMathFuncInsts));
	}

	SpvOpFunctionCall* SpvDebuggerVisitor::AppendVar(const TFunction<int32()>& OffsetEval, SpvPointer* Pointer)
	{
		SpvType* PointeeType = Pointer->Type->PointeeType;
		if (PointeeType->GetKind() == SpvTypeKind::Image || PointeeType->GetKind() == SpvTypeKind::Sampler)
		{
			return nullptr;
		}

		if (CurLine && CurBlock)
		{
			CurBlock->ValidLines.AddUnique(CurLine);
		}

		int32 IndexNum = Pointer->Indexes.Num();
		PatchAppendVarFunc(Pointer, IndexNum);

		SpvId AppendVarFuncId = AppendVarFuncIds[{PointeeType, IndexNum}];
		SpvOpFunctionCall* FuncCall{};
		TArray<TUniquePtr<SpvInstruction>> AppendVarInsts;
		{
			uint32 PackedHeaderValue = PackDebugHeader(SpvDebuggerStateType::VarChange, CurSource.GetValue(), (uint32)CurLine);
			if (CurScope) { Context.HeaderToScope.Add(PackedHeaderValue, CurScope->GetId()); }
			SpvId PackedHeader = Patcher.FindOrAddConstant(PackedHeaderValue);
			SpvId VarId = Patcher.FindOrAddConstant(Pointer->Var->Id.GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> Arguments{ PackedHeader, VarId };
			if (IndexNum > 0)
			{
				SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
				SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
				SpvId ArrType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeArray>(IntType, Patcher.FindOrAddConstant(IndexNum)));
				SpvId IndexArr = Patcher.NewId();
				TArray<SpvId> Constituents;
				for (SpvId Index : Pointer->Indexes)
				{
					SpvId BitCastValue = Patcher.NewId();
					auto BitCastOp = MakeUnique<SpvOpBitcast>(IntType, Index);
					BitCastOp->SetId(BitCastValue);
					AppendVarInsts.Add(MoveTemp(BitCastOp));
					Constituents.Add(BitCastValue);
				}
				auto IndexArrOp = MakeUnique<SpvOpCompositeConstruct>(ArrType, Constituents);
				IndexArrOp->SetId(IndexArr);
				AppendVarInsts.Add(MoveTemp(IndexArrOp));
				Arguments.Add(IndexArr);
			}
			SpvId ParamValue = Patcher.NewId();
			auto ParamValueOp = MakeUnique<SpvOpLoad>(PointeeType->GetId(), Pointer->Id);
			ParamValueOp->SetId(ParamValue);
			AppendVarInsts.Add(MoveTemp(ParamValueOp));
			Arguments.Add(ParamValue);

			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendVarFuncId, Arguments);
			FuncCallOp->SetId(Patcher.NewId());
			FuncCall = FuncCallOp.Get();
			AppendVarInsts.Add(MoveTemp(FuncCallOp));

			PostAppendVar(AppendVarInsts, Pointer, PackedHeader, VarId);
		}
		Patcher.AddInstructions(OffsetEval(), MoveTemp(AppendVarInsts));
		return FuncCall;
	}

	void SpvDebuggerVisitor::AppendAccess(const TFunction<int32()>& OffsetEval, SpvPointer* Pointer)
	{
		int32 IndexNum = Pointer->Indexes.Num();

		TArray<TUniquePtr<SpvInstruction>> AppendAccessInsts;
		{
			uint32 PackedHeaderValue = PackDebugHeader(SpvDebuggerStateType::Access, CurSource.GetValue(), (uint32)CurLine);
			if (CurScope) { Context.HeaderToScope.Add(PackedHeaderValue, CurScope->GetId()); }
			SpvId PackedHeader = Patcher.FindOrAddConstant(PackedHeaderValue);
			SpvId VarId = Patcher.FindOrAddConstant(Pointer->Var->Id.GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());

			PatchAppendAccessFunc(IndexNum);
			SpvId AppendAccessFuncId = AppendAccessFuncIds[IndexNum];
			SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
			TArray<SpvId> Arguments = { PackedHeader, VarId };
			for (SpvId Index : Pointer->Indexes)
			{
				SpvId BitCastIndex = Patcher.NewId();
				auto BitCastOp = MakeUnique<SpvOpBitcast>(IntType, Index);
				BitCastOp->SetId(BitCastIndex);
				AppendAccessInsts.Add(MoveTemp(BitCastOp));
				Arguments.Add(BitCastIndex);
			}
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendAccessFuncId, Arguments);
			FuncCallOp->SetId(Patcher.NewId());
			AppendAccessInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(OffsetEval(), MoveTemp(AppendAccessInsts));
	}

	void SpvDebuggerVisitor::PatchAppendVarFunc(SpvPointer* Pointer, int32 IndexNum)
	{
		SpvPointerType* PointerType = Pointer->Type;
		SpvType* PointeeType = Pointer->Type->PointeeType;
		if (AppendVarFuncIds.Contains({ PointeeType, IndexNum}))
		{
			return;
		}

		SpvId AppendVarFuncId = Patcher.NewId();
		FString FuncName = FString::Printf(TEXT("_AppendVar_%d_%d_"), PointeeType->GetId().GetValue(), IndexNum);
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendVarFuncId, FuncName));
		TArray<TUniquePtr<SpvInstruction>> AppendVarFuncInsts;
		{
			SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> ParamterTypes;
			ParamterTypes.Add(UIntType); // PackedHeader
			ParamterTypes.Add(UIntType); // VarId

			SpvId ArrType;
			if (IndexNum > 0)
			{
				ArrType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeArray>(IntType, Patcher.FindOrAddConstant(IndexNum)));
				ParamterTypes.Add(ArrType);
			}

			ParamterTypes.Add(PointeeType->GetId());

			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, ParamterTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendVarFuncId);
			AppendVarFuncInsts.Add(MoveTemp(FuncOp));

			SpvId PackedHeaderParam = Patcher.NewId();
			auto PackedHeaderParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			PackedHeaderParamOp->SetId(PackedHeaderParam);
			AppendVarFuncInsts.Add(MoveTemp(PackedHeaderParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(PackedHeaderParam, "_DebuggerPackedHeader_"));

			SpvId VarIdParam = Patcher.NewId();
			auto VarIdParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			VarIdParamOp->SetId(VarIdParam);
			AppendVarFuncInsts.Add(MoveTemp(VarIdParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(VarIdParam, "_DebuggerVarId_"));

			SpvId IndexArrParam;
			if (IndexNum > 0)
			{
				IndexArrParam = Patcher.NewId();
				auto IndexArrParamOp = MakeUnique<SpvOpFunctionParameter>(ArrType);
				IndexArrParamOp->SetId(IndexArrParam);
				AppendVarFuncInsts.Add(MoveTemp(IndexArrParamOp));
				Patcher.AddDebugName(MakeUnique<SpvOpName>(IndexArrParam, "_DebuggerIndexes_"));
			}

			SpvId ValueParam = Patcher.NewId();
			auto ValueParamOp = MakeUnique<SpvOpFunctionParameter>(PointeeType->GetId());
			ValueParamOp->SetId(ValueParam);
			AppendVarFuncInsts.Add(MoveTemp(ValueParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(ValueParam, "_DebuggerValue_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendVarFuncInsts.Add(MoveTemp(LabelOp));

			AppendVarFuncIds.Add({ PointeeType, IndexNum }, AppendVarFuncId);
			if (PatchActiveCondition(AppendVarFuncInsts))
			{
				TArray<SpvId> AllValues;
				FlattenToUInts(PackedHeaderParam, UIntType, AppendVarFuncInsts, AllValues);
				FlattenToUInts(VarIdParam, UIntType, AppendVarFuncInsts, AllValues);
				FlattenToUInts(Patcher.FindOrAddConstant((uint32)IndexNum), UIntType, AppendVarFuncInsts, AllValues);
				if (IndexNum > 0)
				{
					FlattenToUInts(IndexArrParam, ArrType, AppendVarFuncInsts, AllValues);
				}
				FlattenToUInts(ValueParam, PointeeType->GetId(), AppendVarFuncInsts, AllValues);
				BatchStoreToDebugBuffer(AllValues, AppendVarFuncInsts);
			}

			AppendVarFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendVarFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendVarFuncInsts));
	}

	void SpvDebuggerVisitor::PatchAppendValueFunc(SpvType* ValueType)
	{
		if (AppendValueFuncIds.Contains(ValueType))
		{
			return;
		}

		SpvId AppendValueFuncId = Patcher.NewId();
		FString FuncName = FString::Printf(TEXT("_AppendValue_%d_"), ValueType->GetId().GetValue());
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendValueFuncId, FuncName));
		TArray<TUniquePtr<SpvInstruction>> AppendValueFuncInsts;
		{
			SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> ParamterTypes;
			ParamterTypes.Add(UIntType); // PackedHeader
			ParamterTypes.Add(ValueType->GetId()); // Value

			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, ParamterTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendValueFuncId);
			AppendValueFuncInsts.Add(MoveTemp(FuncOp));

			SpvId PackedHeaderParam = Patcher.NewId();
			auto PackedHeaderParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			PackedHeaderParamOp->SetId(PackedHeaderParam);
			AppendValueFuncInsts.Add(MoveTemp(PackedHeaderParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(PackedHeaderParam, "_DebuggerPackedHeader_"));

			SpvId ValueParam = Patcher.NewId();
			auto ValueParamOp = MakeUnique<SpvOpFunctionParameter>(ValueType->GetId());
			ValueParamOp->SetId(ValueParam);
			AppendValueFuncInsts.Add(MoveTemp(ValueParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(ValueParam, "_DebuggerValue_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendValueFuncInsts.Add(MoveTemp(LabelOp));

			AppendValueFuncIds.Add(ValueType, AppendValueFuncId);
			if (PatchActiveCondition(AppendValueFuncInsts))
			{
				TArray<SpvId> AllValues;
				FlattenToUInts(PackedHeaderParam, UIntType, AppendValueFuncInsts, AllValues);
				FlattenToUInts(Patcher.FindOrAddConstant((uint32)GetTypeByteSize(ValueType)), UIntType, AppendValueFuncInsts, AllValues);
				FlattenToUInts(ValueParam, ValueType->GetId(), AppendValueFuncInsts, AllValues);
				BatchStoreToDebugBuffer(AllValues, AppendValueFuncInsts);
			}

			AppendValueFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendValueFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendValueFuncInsts));
	}

	void SpvDebuggerVisitor::AppendTag(const TFunction<int32()>& OffsetEval, SpvDebuggerStateType InStateType)
	{
		if (CurLine && CurBlock)
		{
			CurBlock->ValidLines.AddUnique(CurLine);
		}

		TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
		{
			uint32 PackedHeaderValue = PackDebugHeader(InStateType, CurSource.GetValue(), (uint32)CurLine);
			if (CurScope) { Context.HeaderToScope.Add(PackedHeaderValue, CurScope->GetId()); }
			SpvId PackedHeader = Patcher.FindOrAddConstant(PackedHeaderValue);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ PackedHeader });
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(OffsetEval(), MoveTemp(AppendTagInsts));
	}

	void SpvDebuggerVisitor::AppendValue(const TFunction<int32()>& OffsetEval, SpvType* ValueType, SpvId Value, SpvDebuggerStateType InStateType)
	{
		PatchAppendValueFunc(ValueType);
		SpvId AppendValueFuncId = AppendValueFuncIds[ValueType];
		TArray<TUniquePtr<SpvInstruction>> AppendValueInsts;
		{
			uint32 PackedHeaderValue = PackDebugHeader(InStateType, CurSource.GetValue(), (uint32)CurLine);
			if (CurScope) { Context.HeaderToScope.Add(PackedHeaderValue, CurScope->GetId()); }
			SpvId PackedHeader = Patcher.FindOrAddConstant(PackedHeaderValue);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendValueFuncId, TArray<SpvId>{ PackedHeader, Value});
			FuncCallOp->SetId(Patcher.NewId());
			AppendValueInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(OffsetEval(), MoveTemp(AppendValueInsts));
	}

	void SpvDebuggerVisitor::AppendMath(const TFunction<int32()>& OffsetEval, SpvType* ResultType, SpvType* OperandType, const TArray<SpvId>& Operands, SpvDebuggerStateType InStateType)
	{
		int32 OperandNum = Operands.Num();
		PatchAppendMathFunc(ResultType, OperandType, OperandNum);
		SpvId AppendMathFuncId = AppendMathFuncIds[{ResultType, OperandType, OperandNum}];
		TArray<TUniquePtr<SpvInstruction>> AppendMathInsts;
		{
			uint32 PackedHeaderValue = PackDebugHeader(InStateType, CurSource.GetValue(), (uint32)CurLine);
			if (CurScope) { Context.HeaderToScope.Add(PackedHeaderValue, CurScope->GetId()); }
			SpvId PackedHeader = Patcher.FindOrAddConstant(PackedHeaderValue);
			SpvId ResultTypeId = Patcher.FindOrAddConstant(ResultType->GetId().GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> Arguments = { PackedHeader, ResultTypeId };
			Arguments.Append(Operands);
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendMathFuncId, Arguments);
			FuncCallOp->SetId(Patcher.NewId());
			AppendMathInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(OffsetEval(), MoveTemp(AppendMathInsts));
	}

	void SpvDebuggerVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections, const TMap<SpvId, SpvExtSet>& InExtSets)
	{
		this->Insts = &Insts;
		Patcher.SetSpvContext(Insts, SpvCode, &Context);
		//Get entry point loc
		InstIndex = GetInstIndex(this->Insts, Context.EntryPoint);

		// DXC may emit DebugDeclare after the OpLoad/OpStore that references a variable,
		// so pre-scan to populate VariableDescMap for UBSan access checks in OpLoad.
		for (const auto& Inst : Insts)
		{
			if (auto* Declare = dynamic_cast<const SpvDebugDeclare*>(Inst.Get()))
			{
				SpvDebuggerVisitor::Visit(Declare);
			}
		}

		BuildSpvCFG(this->Insts, Context, Context.BBs, Context.Funcs);

		if (EnableUbsan)
		{
			LoadUsedInitUnits = ComputeLoadCheckMasks(this->Insts, Context, Patcher);
			StaticallyInitializedVars = ComputeStaticallyInitializedVars(this->Insts, Context, LoadUsedInitUnits, Context.BBs, Context.Funcs);
		}

		ParseInternal();
	}

	void SpvDebuggerVisitor::ParseInternal()
	{
		//Init global external/location variables
		for (auto& [Id, Var] : Context.GlobalVariables)
		{
			int32 SetNumber = INDEX_NONE;
			int32 BindingNumber = INDEX_NONE;
			TArray<SpvDecoration> Decorations;
			Context.Decorations.MultiFind(Id, Decorations);
			for (const auto& Decoration : Decorations)
			{
				if (Decoration.Kind == SpvDecorationKind::DescriptorSet)
				{
					SetNumber = Decoration.DescriptorSet.Number;
				}
				else if (Decoration.Kind == SpvDecorationKind::Binding)
				{
					BindingNumber = Decoration.Binding.Number;
				}
			}
			if (SetNumber != INDEX_NONE && BindingNumber != INDEX_NONE;
				SpvBinding * Binding = Context.Bindings.FindByPredicate([&](const SpvBinding& InItem) { return InItem.Binding == BindingNumber && InItem.DescriptorSet == SetNumber; }))
			{
				auto& Storage = std::get<SpvObject::External>(Var.Storage);
				Storage.Resource = Binding->Resource;
				if (Binding->Resource->GetType() == GpuResourceType::Buffer)
				{
					GpuBuffer* Buffer = static_cast<GpuBuffer*>(Binding->Resource.GetReference());
					uint8* Data = (uint8*)GGpuRhi->MapGpuBuffer(Buffer, GpuResourceMapMode::Read_Only);
					Storage.Value = { Data, (int)Buffer->GetByteSize() };
					Storage.Resource = Buffer;
					GGpuRhi->UnMapGpuBuffer(Buffer);
					Var.InitializedRanges.AddUnique({ 0, (int)Buffer->GetByteSize() });
				}
			}
		}

		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));

		DebuggerBuffer = PatchDebuggerBuffer(Patcher);
		DebuggerOffset = Patcher.NewId();
		{
			SpvId ZeroU = Patcher.FindOrAddConstant(0u);
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private, ZeroU);
			VarOp->SetId(DebuggerOffset);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerOffset, "_DebuggerOffset_"));
		}

		//Patch AppendTag function
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		AppendTagFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendTagFuncId, "_AppendTag_"));
		TArray<TUniquePtr<SpvInstruction>> AppendTagFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, TArray<SpvId>{ UIntType }));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendTagFuncId);
			AppendTagFuncInsts.Add(MoveTemp(FuncOp));

			SpvId PackedHeaderParam = Patcher.NewId();
			auto PackedHeaderParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			PackedHeaderParamOp->SetId(PackedHeaderParam);
			AppendTagFuncInsts.Add(MoveTemp(PackedHeaderParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(PackedHeaderParam, "_DebuggerPackedHeader_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendTagFuncInsts.Add(MoveTemp(LabelOp));

			if (PatchActiveCondition(AppendTagFuncInsts))
			{
				TArray<SpvId> AllValues;
				FlattenToUInts(PackedHeaderParam, UIntType, AppendTagFuncInsts, AllValues);
				BatchStoreToDebugBuffer(AllValues, AppendTagFuncInsts);
			}

			AppendTagFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendTagFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendTagFuncInsts));

		AppendCallFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendCallFuncId, "_AppendCall_"));
		TArray<TUniquePtr<SpvInstruction>> AppendCallFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, TArray<SpvId>{ UIntType, UIntType }));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendCallFuncId);
			AppendCallFuncInsts.Add(MoveTemp(FuncOp));

			SpvId PackedHeaderParam = Patcher.NewId();
			auto PackedHeaderParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			PackedHeaderParamOp->SetId(PackedHeaderParam);
			AppendCallFuncInsts.Add(MoveTemp(PackedHeaderParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(PackedHeaderParam, "_DebuggerPackedHeader_"));

			SpvId CallIdParam = Patcher.NewId();
			auto CallIdParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			CallIdParamOp->SetId(CallIdParam);
			AppendCallFuncInsts.Add(MoveTemp(CallIdParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(CallIdParam, "_DebuggerCallId_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendCallFuncInsts.Add(MoveTemp(LabelOp));

			if (PatchActiveCondition(AppendCallFuncInsts))
			{
				TArray<SpvId> AllValues;
				FlattenToUInts(PackedHeaderParam, UIntType, AppendCallFuncInsts, AllValues);
				FlattenToUInts(CallIdParam, UIntType, AppendCallFuncInsts, AllValues);
				BatchStoreToDebugBuffer(AllValues, AppendCallFuncInsts);
			}

			AppendCallFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendCallFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendCallFuncInsts));

		while (InstIndex < (*Insts).Num())
		{
			(*Insts)[InstIndex]->Accept(this);
			InstIndex++;
		}
	}
}
