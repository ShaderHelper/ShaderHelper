#include "CommonHeader.h"
#include "SpirvValidator.h"

namespace FW
{
	int32 GetMaxIndexNum(SpvType* Type)
	{
		if (Type->GetKind() == SpvTypeKind::Vector)
		{
			SpvVectorType* VectorType = static_cast<SpvVectorType*>(Type);
			return GetMaxIndexNum(VectorType->ElementType) + 1;
		}
		else if (Type->GetKind() == SpvTypeKind::Struct)
		{
			SpvStructType* StructType = static_cast<SpvStructType*>(Type);
			int32 MaxIndexNum = 0;
			for (int32 Index = 0; Index < StructType->MemberTypes.Num(); Index++)
			{
				SpvType* MemberType = StructType->MemberTypes[Index];
				MaxIndexNum = FMath::Max(MaxIndexNum, GetMaxIndexNum(MemberType));
			}
			return MaxIndexNum + 1;
		}
		else if (Type->GetKind() == SpvTypeKind::Array)
		{
			SpvArrayType* ArrayType = static_cast<SpvArrayType*>(Type);
			return GetMaxIndexNum(ArrayType->ElementType) + 1;
		}
		else if (Type->GetKind() == SpvTypeKind::Matrix)
		{
			SpvMatrixType* MatrixType = static_cast<SpvMatrixType*>(Type);
			return GetMaxIndexNum(MatrixType->ElementType) + 1;
		}
		return 0;
	}

	SpvId SpvValidator::GetScalarOrVectorTypeId(SpvType* ResultType, SpvId ScalarTypeId)
	{
		if (ResultType->IsScalar())
		{
			return ScalarTypeId;
		}
		else
		{
			SpvVectorType* VectorType = static_cast<SpvVectorType*>(ResultType);
			return Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(ScalarTypeId, VectorType->ElementCount));
		}
	}

	void SpvValidator::PatchVarInitializedRange(SpvVariable* Var)
	{
		int32 VarSize = GetTypeByteSize(Var->Type);
		if (VarSize <= 0 || VarSize % 4 != 0)
		{
			return;
		}

		int32 NumUnits = VarSize / 4;
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId InitializedRange = Patcher.NewId();

		SpvId ZeroU = Patcher.FindOrAddConstant(0u);
		if (NumUnits <= 32)
		{
			// Single uint bitmask: 1 bit per 4 bytes, up to 128 bytes
			SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private, ZeroU);
			VarOp->SetId(InitializedRange);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
		}
		else
		{
			// uint[K] array bitmask for variables > 128 bytes
			int32 K = (NumUnits + 31) / 32;
			SpvId ArrType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeArray>(UIntType, Patcher.FindOrAddConstant(K)));
			SpvId ArrPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, ArrType));
			TArray<SpvId> ZeroConstituents;
			for (int32 i = 0; i < K; ++i) ZeroConstituents.Add(ZeroU);
			SpvId ZeroArr = Patcher.FindOrAddConstant(MakeUnique<SpvOpConstantComposite>(ArrType, MoveTemp(ZeroConstituents)));
			auto VarOp = MakeUnique<SpvOpVariable>(ArrPointerPrivateType, SpvStorageClass::Private, ZeroArr);
			VarOp->SetId(InitializedRange);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
		}

		FString VarName = FString::FromInt(Var->Id.GetValue());
		if (Context.Names.contains(Var->Id))
		{
			VarName = Context.Names[Var->Id];
		}
		Patcher.AddDebugName(MakeUnique<SpvOpName>(InitializedRange, FString::Printf(TEXT("_InitializedRange_%s_"), *VarName)));

		VarInitializedRange.Add(Var->Id, InitializedRange);
		VarInitializedRangeUnitCount.Add(Var->Id, NumUnits);
	}

	void SpvValidator::PreAnalyzeInitialization()
	{
		StaticallyInitializedVars = ComputeStaticallyInitializedVars(Insts, Context, LoadUsedInitUnits, BBs, Funcs);
	}

	void SpvValidator::PreAnalyzeLoadUsage()
	{
		LoadUsedInitUnits = ComputeLoadCheckMasks(Insts, Context, Patcher);
	}

	void SpvValidator::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections, const TMap<SpvId, SpvExtSet>& InExtSets)
	{
		this->Insts = &Insts;
		Patcher.SetSpvContext(Insts, SpvCode, &Context);

		// Pre-scan SpvDebugDeclare to populate VariableDescMap
		for (const auto& Inst : Insts)
		{
			if (auto* Declare = dynamic_cast<const SpvDebugDeclare*>(Inst.Get()))
			{
				if (Context.VariableDescs.contains(Declare->GetVarDesc()))
				{
					Context.VariableDescMap.emplace(Declare->GetVariable(), &Context.VariableDescs[Declare->GetVarDesc()]);
				}
			}
		}

		DebuggerBuffer = PatchDebuggerBuffer(Patcher);

		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
		HasError = Patcher.NewId();
		{
			SpvId ZeroU = Patcher.FindOrAddConstant(0u);
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private, ZeroU);
			VarOp->SetId(HasError);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(HasError, "_HasError_"));
		}

		PatchAppendErrorFunc();

		BuildSpvCFG(this->Insts, Context, BBs, Funcs);

		if (EnableUbsan)
		{
			PreAnalyzeLoadUsage();
			PreAnalyzeInitialization();
		}

		// Track uninitialized global Private variables.
		// Uniform/Input/Output/UniformConstant are system-bound and always initialized.
		if (EnableUbsan)
		{
			for (auto& [VarId, Var] : Context.GlobalVariables)
			{
				if (Var.StorageClass != SpvStorageClass::Private)
				{
					continue;
				}
				if (!Context.VariableDescMap.contains(VarId))
				{
					continue;
				}
				PatchVarInitializedRange(&Var);
			}
		}

		{
			int32 InstIndex = GetInstIndex(this->Insts, Context.EntryPoint);
			while (InstIndex < Insts.Num())
			{
				//We don't process function calls until the second pass, because we need the function information processed in the first pass
				if (!dynamic_cast<SpvOpFunctionCall*>(Insts[InstIndex].Get()))
				{
					Insts[InstIndex]->Accept(this);
				}
				InstIndex++;
			}
		}

		{
			int32 InstIndex = GetInstIndex(this->Insts, Context.EntryPoint);
			while (InstIndex < Insts.Num())
			{
				if (dynamic_cast<SpvOpFunctionCall*>(Insts[InstIndex].Get()))
				{
					Insts[InstIndex]->Accept(this);
				}
				InstIndex++;
			}
		}
	
	}

	void SpvValidator::Visit(const SpvOpLabel* Inst)
	{
		CurBlockLabel = Inst->GetId().value();
	}

	void SpvValidator::Visit(const SpvOpPhi* Inst)
	{
		//Block-splitting insertions (SelectionMerge/BranchConditional) make OpPhi
		//parent block references stale. Fix them by replacing old labels with the
		//last continuation label from BlockSplitRemaps.
		int Offset = Inst->GetWordOffset().value();
		int Len = Inst->GetWordLen().value();
		const TArray<uint32>& Spv = Patcher.GetSpv();
		// OpPhi binary: Header(1) | ResultType(1) | ResultId(1) | [Value(1) Parent(1)]*
		for (int i = 4; i < Len; i += 2)
		{
			SpvId ParentId(Spv[Offset + i]);
			if (SpvId* Remap = BlockSplitRemaps.Find(ParentId))
			{
				Patcher.OverwriteWord(Offset + i, Remap->GetValue());
			}
		}
	}

	void SpvValidator::AddBlockSplittingInstructions(int32 Offset, TArray<TUniquePtr<SpvInstruction>>&& InstList)
	{
		if (!InstList.IsEmpty() && CurBlockLabel.IsValid())
		{
			if (auto* Label = dynamic_cast<SpvOpLabel*>(InstList.Last().Get()))
			{
				BlockSplitRemaps.FindOrAdd(CurBlockLabel) = Label->GetId().value();
			}
		}
		Patcher.AddInstructions(Offset, MoveTemp(InstList));
	}

	void SpvValidator::Visit(const SpvOpVariable* Inst)
	{
		SpvId ResultId = Inst->GetId().value();

		SpvStorageClass StorageClass = Inst->GetStorageClass();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = { {ResultId, PointerType->PointeeType}, StorageClass, PointerType };

		TArray<uint8> Value;
		Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
		Var.Storage = SpvObject::Internal{ MoveTemp(Value) };

		LocalVariables.emplace(ResultId, MoveTemp(Var));
		SpvPointer Pointer{
			.Id = ResultId,
			.Var = &LocalVariables[ResultId],
			.Type = PointerType
		};
		LocalPointers.emplace(ResultId, MoveTemp(Pointer));
		if (EnableUbsan && !StaticallyInitializedVars.Contains(ResultId))
		{
			PatchVarInitializedRange(&LocalVariables[ResultId]);
		}
	}

	void SpvValidator::Visit(const SpvOpFunction* Inst)
	{
		CurFunc = &Funcs[Inst->GetId().value()];
	}

	void SpvValidator::Visit(const SpvOpFunctionCall* Inst)
	{
		if (EnableUbsan)
		{
			const SpvFunc& Callee = Funcs[Inst->GetFunction()];
			for (int32 Index = 0; Index < Inst->GetArguments().Num(); Index++)
			{
				SpvId Argument = Inst->GetArguments()[Index];
				SpvId Parameter = Callee.Parameters[Index];
				if (!VarInitializedRange.Contains(Argument) || !VarInitializedRange.Contains(Parameter))
				{
					continue;
				}

				SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
				int32 NumUnits = VarInitializedRangeUnitCount[Argument];

				if (NumUnits <= 32)
				{
					// Single uint: copy directly
					// Before call: copy arg bitmask → param bitmask
					{
						TArray<TUniquePtr<SpvInstruction>> InstList;
						SpvId LoadedArgument = Patcher.NewId();
						auto LoadedArgumentOp = MakeUnique<SpvOpLoad>(UIntType, VarInitializedRange[Argument]);
						LoadedArgumentOp->SetId(LoadedArgument);
						InstList.Add(MoveTemp(LoadedArgumentOp));

						InstList.Add(MakeUnique<SpvOpStore>(VarInitializedRange[Parameter], LoadedArgument));

						AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(InstList));
					}
				
					// After call: copy param bitmask → arg bitmask
					{
						TArray<TUniquePtr<SpvInstruction>> InstList;
						SpvId LoadedParameter = Patcher.NewId();
						auto LoadedParameterOp = MakeUnique<SpvOpLoad>(UIntType, VarInitializedRange[Parameter]);
						LoadedParameterOp->SetId(LoadedParameter);
						InstList.Add(MoveTemp(LoadedParameterOp));

						InstList.Add(MakeUnique<SpvOpStore>(VarInitializedRange[Argument], LoadedParameter));

						AddBlockSplittingInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(InstList));
					}
				}
				else
				{
					// Array bitmask: copy each element
					int32 K = (NumUnits + 31) / 32;
					SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));

					// Before call: copy arg array → param array
					{
						TArray<TUniquePtr<SpvInstruction>> InstList;
						for (int32 i = 0; i < K; i++)
						{
							SpvId ArgElemPtr = Patcher.NewId();
							auto ArgElemPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[Argument], TArray<SpvId>{Patcher.FindOrAddConstant((uint32)i)});
							ArgElemPtrOp->SetId(ArgElemPtr);
							InstList.Add(MoveTemp(ArgElemPtrOp));

							SpvId LoadedElem = Patcher.NewId();
							auto LoadedElemOp = MakeUnique<SpvOpLoad>(UIntType, ArgElemPtr);
							LoadedElemOp->SetId(LoadedElem);
							InstList.Add(MoveTemp(LoadedElemOp));

							SpvId ParamElemPtr = Patcher.NewId();
							auto ParamElemPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[Parameter], TArray<SpvId>{Patcher.FindOrAddConstant((uint32)i)});
							ParamElemPtrOp->SetId(ParamElemPtr);
							InstList.Add(MoveTemp(ParamElemPtrOp));

							InstList.Add(MakeUnique<SpvOpStore>(ParamElemPtr, LoadedElem));
						}
						AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(InstList));
					}

					// After call: copy param array → arg array
					{
						TArray<TUniquePtr<SpvInstruction>> InstList;
						for (int32 i = 0; i < K; i++)
						{
							SpvId ParamElemPtr = Patcher.NewId();
							auto ParamElemPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[Parameter], TArray<SpvId>{Patcher.FindOrAddConstant((uint32)i)});
							ParamElemPtrOp->SetId(ParamElemPtr);
							InstList.Add(MoveTemp(ParamElemPtrOp));

							SpvId LoadedElem = Patcher.NewId();
							auto LoadedElemOp = MakeUnique<SpvOpLoad>(UIntType, ParamElemPtr);
							LoadedElemOp->SetId(LoadedElem);
							InstList.Add(MoveTemp(LoadedElemOp));

							SpvId ArgElemPtr = Patcher.NewId();
							auto ArgElemPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[Argument], TArray<SpvId>{Patcher.FindOrAddConstant((uint32)i)});
							ArgElemPtrOp->SetId(ArgElemPtr);
							InstList.Add(MoveTemp(ArgElemPtrOp));

							InstList.Add(MakeUnique<SpvOpStore>(ArgElemPtr, LoadedElem));
						}
						AddBlockSplittingInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(InstList));
					}
				}
			}
		}
	}

	void SpvValidator::Visit(const SpvOpFunctionParameter* Inst)
	{
		SpvId ResultId = Inst->GetId().value();

		SpvType* ParamType = Context.Types[Inst->GetResultType()].Get();
		if (ParamType->GetKind() == SpvTypeKind::Pointer)
		{
			SpvPointerType* PointerType = static_cast<SpvPointerType*>(ParamType);
			SpvVariable Var = { {ResultId, PointerType->PointeeType}, SpvStorageClass::Function, PointerType };

			TArray<uint8> Value;
			Value.SetNumZeroed(GetTypeByteSize(PointerType->PointeeType));
			Var.Storage = SpvObject::Internal{ MoveTemp(Value) };

			LocalVariables.emplace(ResultId, MoveTemp(Var));
			SpvPointer Pointer{
				.Id = ResultId,
				.Var = &LocalVariables[ResultId],
				.Type = PointerType
			};
			LocalPointers.emplace(ResultId, MoveTemp(Pointer));
		}
		else
		{
			// Value-type parameter (passed by value, not pointer)
			SpvVariable Var = { {ResultId, ParamType}, SpvStorageClass::Function };

			TArray<uint8> Value;
			Value.SetNumZeroed(GetTypeByteSize(ParamType));
			Var.Storage = SpvObject::Internal{ MoveTemp(Value) };

			LocalVariables.emplace(ResultId, MoveTemp(Var));
		}
		if (EnableUbsan)
		{
			PatchVarInitializedRange(&LocalVariables[ResultId]);
		}
	}

	void SpvValidator::Visit(const SpvOpLoad* Inst)
	{
		SpvPointer* Pointer = FindPointer(Inst->GetPointer());
		if (!Pointer)
		{
			return;
		}
		if (!VarInitializedRange.Contains(Pointer->Var->Id))
		{
			// No VarInitializedRange: static analysis confirmed the variable is fully initialized,
			// but dynamic indexes may still be out of bounds. Use GetAccess for bounds checking.
			if (EnableUbsan && !Pointer->Indexes.IsEmpty())
			{
				SpvType* VarType = Pointer->Var->Type;
				TArray<TUniquePtr<SpvInstruction>> BoundsCheckInsts;
				GetAccess(VarType, Pointer->Indexes, BoundsCheckInsts);
				if (BoundsCheckInsts.Num() > 0)
				{
					AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(BoundsCheckInsts));
				}
			}
			return;
		}

		if (EnableUbsan)
		{
			SpvType* VarType = Pointer->Var->Type;
			SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			int32 NumUnits = VarInitializedRangeUnitCount[Pointer->Var->Id];
			int32 Count = GetTypeByteSize(Pointer->Type->PointeeType) / 4;
			TArray<TUniquePtr<SpvInstruction>> ValidateInsts;

			if (NumUnits <= 32)
			{
				// Single uint bitmask path
				SpvId StartInitUnit = GetAccess(VarType, Pointer->Indexes, ValidateInsts);

				uint32 RawMask = ((1u << Count) - 1u);

				// Narrow the mask if only a subset of the composite's init-units are actually used.
				// e.g. glslang compiles c.xyz as OpLoad vec4 + OpVectorShuffle 0 1 2,
				// so we only need bits 0-2 instead of all 4. Also handles structs/arrays/matrices.
				if (uint32* UsedMask = LoadUsedInitUnits.Find(Inst->GetId().value()))
				{
					RawMask = *UsedMask;
				}

				SpvId RawMaskConst = Patcher.FindOrAddConstant(RawMask);

				SpvId Mask = Patcher.NewId();
				auto MaskOp = MakeUnique<SpvOpShiftLeftLogical>(UIntType, RawMaskConst, StartInitUnit);
				MaskOp->SetId(Mask);
				ValidateInsts.Add(MoveTemp(MaskOp));

				SpvId Loaded = Patcher.NewId();
				auto LoadOp = MakeUnique<SpvOpLoad>(UIntType, VarInitializedRange[Pointer->Var->Id]);
				LoadOp->SetId(Loaded);
				ValidateInsts.Add(MoveTemp(LoadOp));

				SpvId Masked = Patcher.NewId();
				auto AndOp = MakeUnique<SpvOpBitwiseAnd>(UIntType, Loaded, Mask);
				AndOp->SetId(Masked);
				ValidateInsts.Add(MoveTemp(AndOp));

				SpvId NotFullyInit = Patcher.NewId();
				auto NotEqualOp = MakeUnique<SpvOpINotEqual>(BoolType, Masked, Mask);
				NotEqualOp->SetId(NotFullyInit);
				ValidateInsts.Add(MoveTemp(NotEqualOp));

				auto TrueLabel = Patcher.NewId();
				auto MergeLabel = Patcher.NewId();
				ValidateInsts.Add(MakeUnique<SpvOpSelectionMerge>(MergeLabel, SpvSelectionControl::None));
				ValidateInsts.Add(MakeUnique<SpvOpBranchConditional>(NotFullyInit, TrueLabel, MergeLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ValidateInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ValidateInsts);
				ValidateInsts.Add(MakeUnique<SpvOpBranch>(MergeLabel));

				auto MergeLabelOp = MakeUnique<SpvOpLabel>();
				MergeLabelOp->SetId(MergeLabel);
				ValidateInsts.Add(MoveTemp(MergeLabelOp));
			}
			else if (Count > 32)
			{
				// Full/large access on array bitmask: unroll check over all K elements
				int32 K = (NumUnits + 31) / 32;
				SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
				for (int32 i = 0; i < K; i++)
				{
					int32 Remaining = (i < K - 1) ? 32 : ((NumUnits % 32 == 0) ? 32 : (NumUnits % 32));
					uint32 ExpectedMask = ((1u << Remaining) - 1u);

					SpvId ElemPtr = Patcher.NewId();
					auto ElemPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[Pointer->Var->Id], TArray<SpvId>{Patcher.FindOrAddConstant((uint32)i)});
					ElemPtrOp->SetId(ElemPtr);
					ValidateInsts.Add(MoveTemp(ElemPtrOp));

					SpvId ElemLoaded = Patcher.NewId();
					auto ElemLoadOp = MakeUnique<SpvOpLoad>(UIntType, ElemPtr);
					ElemLoadOp->SetId(ElemLoaded);
					ValidateInsts.Add(MoveTemp(ElemLoadOp));

					SpvId NotFull = Patcher.NewId();
					auto NotFullOp = MakeUnique<SpvOpINotEqual>(BoolType, ElemLoaded, Patcher.FindOrAddConstant(ExpectedMask));
					NotFullOp->SetId(NotFull);
					ValidateInsts.Add(MoveTemp(NotFullOp));

					auto ErrorLabel = Patcher.NewId();
					auto ElemMerge = Patcher.NewId();
					ValidateInsts.Add(MakeUnique<SpvOpSelectionMerge>(ElemMerge, SpvSelectionControl::None));
					ValidateInsts.Add(MakeUnique<SpvOpBranchConditional>(NotFull, ErrorLabel, ElemMerge));

					auto ErrorLabelOp = MakeUnique<SpvOpLabel>();
					ErrorLabelOp->SetId(ErrorLabel);
					ValidateInsts.Add(MoveTemp(ErrorLabelOp));
					AppendError(ValidateInsts);
					ValidateInsts.Add(MakeUnique<SpvOpBranch>(ElemMerge));

					auto ElemMergeOp = MakeUnique<SpvOpLabel>();
					ElemMergeOp->SetId(ElemMerge);
					ValidateInsts.Add(MoveTemp(ElemMergeOp));
				}
			}
			else
			{
				// Sub-member access on array bitmask (Count <= 32)
				SpvId StartInitUnit = GetAccess(VarType, Pointer->Indexes, ValidateInsts);
				SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));

				// elemIdx = StartInitUnit >> 5
				SpvId ElemIdx = Patcher.NewId();
				auto ElemIdxOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, StartInitUnit, Patcher.FindOrAddConstant(5u));
				ElemIdxOp->SetId(ElemIdx);
				ValidateInsts.Add(MoveTemp(ElemIdxOp));

				// bitPos = StartInitUnit & 31
				SpvId BitPos = Patcher.NewId();
				auto BitPosOp = MakeUnique<SpvOpBitwiseAnd>(UIntType, StartInitUnit, Patcher.FindOrAddConstant(31u));
				BitPosOp->SetId(BitPos);
				ValidateInsts.Add(MoveTemp(BitPosOp));

				// ptr = AccessChain(array, elemIdx)
				SpvId ElemPtr = Patcher.NewId();
				auto ElemPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[Pointer->Var->Id], TArray<SpvId>{ElemIdx});
				ElemPtrOp->SetId(ElemPtr);
				ValidateInsts.Add(MoveTemp(ElemPtrOp));

				// loaded = Load(ptr)
				SpvId Loaded = Patcher.NewId();
				auto LoadOp = MakeUnique<SpvOpLoad>(UIntType, ElemPtr);
				LoadOp->SetId(Loaded);
				ValidateInsts.Add(MoveTemp(LoadOp));

				// mask = rawMask << bitPos
				uint32 RawMask = ((1u << Count) - 1u);
				if (uint32* UsedMask = LoadUsedInitUnits.Find(Inst->GetId().value()))
				{
					RawMask = *UsedMask;
				}
				SpvId RawMaskConst = Patcher.FindOrAddConstant(RawMask);
				SpvId Mask = Patcher.NewId();
				auto MaskOp = MakeUnique<SpvOpShiftLeftLogical>(UIntType, RawMaskConst, BitPos);
				MaskOp->SetId(Mask);
				ValidateInsts.Add(MoveTemp(MaskOp));

				// masked = loaded & mask
				SpvId Masked = Patcher.NewId();
				auto AndOp = MakeUnique<SpvOpBitwiseAnd>(UIntType, Loaded, Mask);
				AndOp->SetId(Masked);
				ValidateInsts.Add(MoveTemp(AndOp));

				// notFullyInit = (masked != mask)
				SpvId NotFullyInit = Patcher.NewId();
				auto NotEqualOp = MakeUnique<SpvOpINotEqual>(BoolType, Masked, Mask);
				NotEqualOp->SetId(NotFullyInit);
				ValidateInsts.Add(MoveTemp(NotEqualOp));

				// if (notFullyInit) AppendError()
				auto PrimaryErrorLabel = Patcher.NewId();
				auto PrimaryMergeLabel = Patcher.NewId();
				ValidateInsts.Add(MakeUnique<SpvOpSelectionMerge>(PrimaryMergeLabel, SpvSelectionControl::None));
				ValidateInsts.Add(MakeUnique<SpvOpBranchConditional>(NotFullyInit, PrimaryErrorLabel, PrimaryMergeLabel));

				auto PrimaryErrorLabelOp = MakeUnique<SpvOpLabel>();
				PrimaryErrorLabelOp->SetId(PrimaryErrorLabel);
				ValidateInsts.Add(MoveTemp(PrimaryErrorLabelOp));
				AppendError(ValidateInsts);
				ValidateInsts.Add(MakeUnique<SpvOpBranch>(PrimaryMergeLabel));

				auto PrimaryMergeLabelOp = MakeUnique<SpvOpLabel>();
				PrimaryMergeLabelOp->SetId(PrimaryMergeLabel);
				ValidateInsts.Add(MoveTemp(PrimaryMergeLabelOp));

				// Handle potential spanning into next element (only when Count > 1)
				if (Count > 1)
				{
					// hasOverflow = (bitPos + Count > 32) iff (bitPos > 32 - Count)
					SpvId HasOverflow = Patcher.NewId();
					auto HasOverflowOp = MakeUnique<SpvOpUGreaterThan>(BoolType, BitPos, Patcher.FindOrAddConstant((uint32)(32 - Count)));
					HasOverflowOp->SetId(HasOverflow);
					ValidateInsts.Add(MoveTemp(HasOverflowOp));

					auto OverflowBodyLabel = Patcher.NewId();
					auto OverflowMergeLabel = Patcher.NewId();
					ValidateInsts.Add(MakeUnique<SpvOpSelectionMerge>(OverflowMergeLabel, SpvSelectionControl::None));
					ValidateInsts.Add(MakeUnique<SpvOpBranchConditional>(HasOverflow, OverflowBodyLabel, OverflowMergeLabel));

					auto OverflowBodyLabelOp = MakeUnique<SpvOpLabel>();
					OverflowBodyLabelOp->SetId(OverflowBodyLabel);
					ValidateInsts.Add(MoveTemp(OverflowBodyLabelOp));
					{
						// elemIdx2 = elemIdx + 1
						SpvId ElemIdx2 = Patcher.NewId();
						auto ElemIdx2Op = MakeUnique<SpvOpIAdd>(UIntType, ElemIdx, Patcher.FindOrAddConstant(1u));
						ElemIdx2Op->SetId(ElemIdx2);
						ValidateInsts.Add(MoveTemp(ElemIdx2Op));

						// ptr2 = AccessChain(array, elemIdx2)
						SpvId ElemPtr2 = Patcher.NewId();
						auto ElemPtr2Op = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[Pointer->Var->Id], TArray<SpvId>{ElemIdx2});
						ElemPtr2Op->SetId(ElemPtr2);
						ValidateInsts.Add(MoveTemp(ElemPtr2Op));

						// loaded2 = Load(ptr2)
						SpvId Loaded2 = Patcher.NewId();
						auto Load2Op = MakeUnique<SpvOpLoad>(UIntType, ElemPtr2);
						Load2Op->SetId(Loaded2);
						ValidateInsts.Add(MoveTemp(Load2Op));

						// shiftRight = 32 - bitPos (safe: hasOverflow guarantees bitPos > 0)
						SpvId ShiftRight = Patcher.NewId();
						auto ShiftRightOp = MakeUnique<SpvOpISub>(UIntType, Patcher.FindOrAddConstant(32u), BitPos);
						ShiftRightOp->SetId(ShiftRight);
						ValidateInsts.Add(MoveTemp(ShiftRightOp));

						// mask2 = rawMask >> shiftRight
						SpvId Mask2 = Patcher.NewId();
						auto Mask2Op = MakeUnique<SpvOpShiftRightLogical>(UIntType, RawMaskConst, ShiftRight);
						Mask2Op->SetId(Mask2);
						ValidateInsts.Add(MoveTemp(Mask2Op));

						// masked2 = loaded2 & mask2
						SpvId Masked2 = Patcher.NewId();
						auto And2Op = MakeUnique<SpvOpBitwiseAnd>(UIntType, Loaded2, Mask2);
						And2Op->SetId(Masked2);
						ValidateInsts.Add(MoveTemp(And2Op));

						// notFullyInit2 = (masked2 != mask2)
						SpvId NotFullyInit2 = Patcher.NewId();
						auto NotEqual2Op = MakeUnique<SpvOpINotEqual>(BoolType, Masked2, Mask2);
						NotEqual2Op->SetId(NotFullyInit2);
						ValidateInsts.Add(MoveTemp(NotEqual2Op));

						// if (notFullyInit2) AppendError()
						auto OverflowErrorLabel = Patcher.NewId();
						auto OverflowErrorMerge = Patcher.NewId();
						ValidateInsts.Add(MakeUnique<SpvOpSelectionMerge>(OverflowErrorMerge, SpvSelectionControl::None));
						ValidateInsts.Add(MakeUnique<SpvOpBranchConditional>(NotFullyInit2, OverflowErrorLabel, OverflowErrorMerge));

						auto OverflowErrorLabelOp = MakeUnique<SpvOpLabel>();
						OverflowErrorLabelOp->SetId(OverflowErrorLabel);
						ValidateInsts.Add(MoveTemp(OverflowErrorLabelOp));
						AppendError(ValidateInsts);
						ValidateInsts.Add(MakeUnique<SpvOpBranch>(OverflowErrorMerge));

						auto OverflowErrorMergeOp = MakeUnique<SpvOpLabel>();
						OverflowErrorMergeOp->SetId(OverflowErrorMerge);
						ValidateInsts.Add(MoveTemp(OverflowErrorMergeOp));
					}
					ValidateInsts.Add(MakeUnique<SpvOpBranch>(OverflowMergeLabel));

					auto OverflowMergeLabelOp = MakeUnique<SpvOpLabel>();
					OverflowMergeLabelOp->SetId(OverflowMergeLabel);
					ValidateInsts.Add(MoveTemp(OverflowMergeLabelOp));
				}
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(ValidateInsts));
		}
	}

	void SpvValidator::Visit(const SpvOpStore* Inst)
	{
		SpvPointer* Pointer = FindPointer(Inst->GetPointer());
		if (!Pointer)
		{
			return;
		}
		if (!VarInitializedRange.Contains(Pointer->Var->Id))
		{
			if (EnableUbsan && !Pointer->Indexes.IsEmpty())
			{
				SpvType* VarType = Pointer->Var->Type;
				TArray<TUniquePtr<SpvInstruction>> BoundsCheckInsts;
				GetAccess(VarType, Pointer->Indexes, BoundsCheckInsts);
				if (BoundsCheckInsts.Num() > 0)
				{
					AddBlockSplittingInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(BoundsCheckInsts));
				}
			}
			return;
		}

		if (EnableUbsan)
		{
			SpvType* VarType = Pointer->Var->Type;
			SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			int32 NumUnits = VarInitializedRangeUnitCount[Pointer->Var->Id];
			int32 Count = GetTypeByteSize(Pointer->Type->PointeeType) / 4;
			uint32 RawMask = ((1u << Count) - 1u);

			// Detect VectorShuffle read-modify-write pattern (swizzle write).
			// Only mark the actually-written components as initialized.
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
								RawMask = WriteMask;
							}
						}
					}
				}
			}

			TArray<TUniquePtr<SpvInstruction>> UpdateInsts;

			if (NumUnits <= 32)
			{
				// Single uint bitmask path
				SpvId StartInitUnit = GetAccess(VarType, Pointer->Indexes, UpdateInsts);

				SpvId RawMaskConst = Patcher.FindOrAddConstant(RawMask);

				SpvId Mask = Patcher.NewId();
				auto MaskOp = MakeUnique<SpvOpShiftLeftLogical>(UIntType, RawMaskConst, StartInitUnit);
				MaskOp->SetId(Mask);
				UpdateInsts.Add(MoveTemp(MaskOp));

				SpvId Loaded = Patcher.NewId();
				auto LoadOp = MakeUnique<SpvOpLoad>(UIntType, VarInitializedRange[Pointer->Var->Id]);
				LoadOp->SetId(Loaded);
				UpdateInsts.Add(MoveTemp(LoadOp));

				SpvId Updated = Patcher.NewId();
				auto OrOp = MakeUnique<SpvOpBitwiseOr>(UIntType, Loaded, Mask);
				OrOp->SetId(Updated);
				UpdateInsts.Add(MoveTemp(OrOp));

				UpdateInsts.Add(MakeUnique<SpvOpStore>(VarInitializedRange[Pointer->Var->Id], Updated));
			}
			else if (Count > 32)
			{
				// Full/large store on array bitmask: unroll store over all K elements
				int32 K = (NumUnits + 31) / 32;
				SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
				for (int32 i = 0; i < K; i++)
				{
					int32 Remaining = (i < K - 1) ? 32 : ((NumUnits % 32 == 0) ? 32 : (NumUnits % 32));
					uint32 FullMask = ((1u << Remaining) - 1u);

					SpvId ElemPtr = Patcher.NewId();
					auto ElemPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[Pointer->Var->Id], TArray<SpvId>{Patcher.FindOrAddConstant((uint32)i)});
					ElemPtrOp->SetId(ElemPtr);
					UpdateInsts.Add(MoveTemp(ElemPtrOp));

					UpdateInsts.Add(MakeUnique<SpvOpStore>(ElemPtr, Patcher.FindOrAddConstant(FullMask)));
				}
			}
			else
			{
				// Sub-member store on array bitmask (Count <= 32)
				SpvId StartInitUnit = GetAccess(VarType, Pointer->Indexes, UpdateInsts);
				SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));

				// elemIdx = StartInitUnit >> 5
				SpvId ElemIdx = Patcher.NewId();
				auto ElemIdxOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, StartInitUnit, Patcher.FindOrAddConstant(5u));
				ElemIdxOp->SetId(ElemIdx);
				UpdateInsts.Add(MoveTemp(ElemIdxOp));

				// bitPos = StartInitUnit & 31
				SpvId BitPos = Patcher.NewId();
				auto BitPosOp = MakeUnique<SpvOpBitwiseAnd>(UIntType, StartInitUnit, Patcher.FindOrAddConstant(31u));
				BitPosOp->SetId(BitPos);
				UpdateInsts.Add(MoveTemp(BitPosOp));

				// ptr = AccessChain(array, elemIdx)
				SpvId ElemPtr = Patcher.NewId();
				auto ElemPtrOp = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[Pointer->Var->Id], TArray<SpvId>{ElemIdx});
				ElemPtrOp->SetId(ElemPtr);
				UpdateInsts.Add(MoveTemp(ElemPtrOp));

				// loaded = Load(ptr)
				SpvId Loaded = Patcher.NewId();
				auto LoadOp = MakeUnique<SpvOpLoad>(UIntType, ElemPtr);
				LoadOp->SetId(Loaded);
				UpdateInsts.Add(MoveTemp(LoadOp));

				// mask = rawMask << bitPos
				SpvId RawMaskConst = Patcher.FindOrAddConstant(RawMask);
				SpvId Mask = Patcher.NewId();
				auto MaskOp = MakeUnique<SpvOpShiftLeftLogical>(UIntType, RawMaskConst, BitPos);
				MaskOp->SetId(Mask);
				UpdateInsts.Add(MoveTemp(MaskOp));

				// updated = loaded | mask
				SpvId Updated = Patcher.NewId();
				auto OrOp = MakeUnique<SpvOpBitwiseOr>(UIntType, Loaded, Mask);
				OrOp->SetId(Updated);
				UpdateInsts.Add(MoveTemp(OrOp));

				// store back
				UpdateInsts.Add(MakeUnique<SpvOpStore>(ElemPtr, Updated));

				// Handle potential spanning into next element (only when Count > 1)
				if (Count > 1)
				{
					SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());

					// hasOverflow = (bitPos > 32 - Count)
					SpvId HasOverflow = Patcher.NewId();
					auto HasOverflowOp = MakeUnique<SpvOpUGreaterThan>(BoolType, BitPos, Patcher.FindOrAddConstant((uint32)(32 - Count)));
					HasOverflowOp->SetId(HasOverflow);
					UpdateInsts.Add(MoveTemp(HasOverflowOp));

					auto OverflowBodyLabel = Patcher.NewId();
					auto OverflowMergeLabel = Patcher.NewId();
					UpdateInsts.Add(MakeUnique<SpvOpSelectionMerge>(OverflowMergeLabel, SpvSelectionControl::None));
					UpdateInsts.Add(MakeUnique<SpvOpBranchConditional>(HasOverflow, OverflowBodyLabel, OverflowMergeLabel));

					auto OverflowBodyLabelOp = MakeUnique<SpvOpLabel>();
					OverflowBodyLabelOp->SetId(OverflowBodyLabel);
					UpdateInsts.Add(MoveTemp(OverflowBodyLabelOp));
					{
						// elemIdx2 = elemIdx + 1
						SpvId ElemIdx2 = Patcher.NewId();
						auto ElemIdx2Op = MakeUnique<SpvOpIAdd>(UIntType, ElemIdx, Patcher.FindOrAddConstant(1u));
						ElemIdx2Op->SetId(ElemIdx2);
						UpdateInsts.Add(MoveTemp(ElemIdx2Op));

						// ptr2 = AccessChain(array, elemIdx2)
						SpvId ElemPtr2 = Patcher.NewId();
						auto ElemPtr2Op = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[Pointer->Var->Id], TArray<SpvId>{ElemIdx2});
						ElemPtr2Op->SetId(ElemPtr2);
						UpdateInsts.Add(MoveTemp(ElemPtr2Op));

						// loaded2 = Load(ptr2)
						SpvId Loaded2 = Patcher.NewId();
						auto Load2Op = MakeUnique<SpvOpLoad>(UIntType, ElemPtr2);
						Load2Op->SetId(Loaded2);
						UpdateInsts.Add(MoveTemp(Load2Op));

						// shiftRight = 32 - bitPos
						SpvId ShiftRight = Patcher.NewId();
						auto ShiftRightOp = MakeUnique<SpvOpISub>(UIntType, Patcher.FindOrAddConstant(32u), BitPos);
						ShiftRightOp->SetId(ShiftRight);
						UpdateInsts.Add(MoveTemp(ShiftRightOp));

						// mask2 = rawMask >> shiftRight
						SpvId Mask2 = Patcher.NewId();
						auto Mask2Op = MakeUnique<SpvOpShiftRightLogical>(UIntType, RawMaskConst, ShiftRight);
						Mask2Op->SetId(Mask2);
						UpdateInsts.Add(MoveTemp(Mask2Op));

						// updated2 = loaded2 | mask2
						SpvId Updated2 = Patcher.NewId();
						auto Or2Op = MakeUnique<SpvOpBitwiseOr>(UIntType, Loaded2, Mask2);
						Or2Op->SetId(Updated2);
						UpdateInsts.Add(MoveTemp(Or2Op));

						// store back
						UpdateInsts.Add(MakeUnique<SpvOpStore>(ElemPtr2, Updated2));
					}
					UpdateInsts.Add(MakeUnique<SpvOpBranch>(OverflowMergeLabel));

					auto OverflowMergeLabelOp = MakeUnique<SpvOpLabel>();
					OverflowMergeLabelOp->SetId(OverflowMergeLabel);
					UpdateInsts.Add(MoveTemp(OverflowMergeLabelOp));
				}
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(UpdateInsts));
		}
	}

	void SpvValidator::Visit(const SpvOpAccessChain* Inst)
	{
		SpvPointer* BasePointer = FindPointer(Inst->GetBasePointer());
		if (!BasePointer)
		{
			return;
		}

		SpvId ResultId = Inst->GetId().value();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		TArray<SpvId> Indexes = BasePointer->Indexes;
		Indexes.Append(Inst->GetIndexes());

		SpvPointer Pointer{
			.Id = ResultId,
			.Var = BasePointer->Var,
			.Indexes = MoveTemp(Indexes),
			.Type = PointerType
		};
		LocalPointers.emplace(ResultId, MoveTemp(Pointer));
	}

	void SpvValidator::Visit(const SpvPow* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			TArray<TUniquePtr<SpvInstruction>> PowValidationInsts;
			{
				SpvId Zero = GetScalarOrVectorId(ResultType, 0.0f);
				SpvId FOrdLessThan = Patcher.NewId();
				auto FOrdLessThanOp = MakeUnique<SpvOpFOrdLessThan>(BoolResultType, Inst->GetX(), Zero);
				FOrdLessThanOp->SetId(FOrdLessThan);
				PowValidationInsts.Add(MoveTemp(FOrdLessThanOp));

				SpvId FOrdEqual = Patcher.NewId();
				auto FOrdEqualOp = MakeUnique<SpvOpFOrdEqual>(BoolResultType, Inst->GetX(), Zero);
				FOrdEqualOp->SetId(FOrdEqual);
				PowValidationInsts.Add(MoveTemp(FOrdEqualOp));

				SpvId FOrdLessThanEqual = Patcher.NewId();
				auto FOrdLessThanEqualOp = MakeUnique<SpvOpFOrdLessThanEqual>(BoolResultType, Inst->GetY(), Zero);
				FOrdLessThanEqualOp->SetId(FOrdLessThanEqual);
				PowValidationInsts.Add(MoveTemp(FOrdLessThanEqualOp));

				SpvId LogicalAnd = Patcher.NewId();
				auto LogicalAndOp = MakeUnique<SpvOpLogicalAnd>(BoolResultType, FOrdEqual, FOrdLessThanEqual);
				LogicalAndOp->SetId(LogicalAnd);
				PowValidationInsts.Add(MoveTemp(LogicalAndOp));

				SpvId LogicalOr = Patcher.NewId();
				auto LogicalOrOp = MakeUnique<SpvOpLogicalOr>(BoolResultType, FOrdLessThan, LogicalAnd);
				LogicalOrOp->SetId(LogicalOr);
				PowValidationInsts.Add(MoveTemp(LogicalOrOp));

				SpvId Condition = LogicalOr;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, LogicalOr);
					AnyOp->SetId(Any);
					PowValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				PowValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				PowValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				PowValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(PowValidationInsts);
				PowValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				PowValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(PowValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvNormalize* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
			TArray<TUniquePtr<SpvInstruction>> NormalizeValidationInsts;
			{
				SpvId Zero = GetScalarOrVectorId(ResultType, 0.0f);
				SpvId FOrdEqual = Patcher.NewId();;
				auto FOrdEqualOp = MakeUnique<SpvOpFOrdEqual>(BoolResultType, Inst->GetX(), Zero);
				FOrdEqualOp->SetId(FOrdEqual);
				NormalizeValidationInsts.Add(MoveTemp(FOrdEqualOp));

				SpvId Condition = FOrdEqual;
				if (!ResultType->IsScalar())
				{
					SpvId All = Patcher.NewId();
					auto AllOp = MakeUnique<SpvOpAll>(BoolType, FOrdEqual);
					AllOp->SetId(All);
					NormalizeValidationInsts.Add(MoveTemp(AllOp));
					Condition = All;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				NormalizeValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				NormalizeValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				NormalizeValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(NormalizeValidationInsts);
				NormalizeValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				NormalizeValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(NormalizeValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvLog* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyComparisonZeroError([&] {return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetX(), SpvOp::FOrdLessThanEqual);
		}
	}

	void SpvValidator::Visit(const SpvLog2* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyComparisonZeroError([&] {return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetX(), SpvOp::FOrdLessThanEqual);
		}
	}

	void SpvValidator::Visit(const SpvAsin* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
			SpvId FloatResultType = GetScalarOrVectorTypeId(ResultType, FloatType);
			TArray<TUniquePtr<SpvInstruction>> AsinValidationInsts;
			{
				SpvId One = GetScalarOrVectorId(ResultType, 1.0f);

				SpvId ExtSet450 = *Context.ExtSets.FindKey(SpvExtSet::GLSLstd450);
				SpvId FAbs = Patcher.NewId();
				auto FAbsOp = MakeUnique<SpvFAbs>(FloatResultType, ExtSet450, Inst->GetX());
				FAbsOp->SetId(FAbs);
				AsinValidationInsts.Add(MoveTemp(FAbsOp));

				SpvId FOrdGreaterThan = Patcher.NewId();
				auto FOrdGreaterThanOp = MakeUnique<SpvOpFOrdGreaterThan>(BoolResultType, FAbs, One);
				FOrdGreaterThanOp->SetId(FOrdGreaterThan);
				AsinValidationInsts.Add(MoveTemp(FOrdGreaterThanOp));

				SpvId Condition = FOrdGreaterThan;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, FOrdGreaterThan);
					AnyOp->SetId(Any);
					AsinValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				AsinValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				AsinValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				AsinValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(AsinValidationInsts);
				AsinValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				AsinValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(AsinValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvAcos* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
			SpvId FloatResultType = GetScalarOrVectorTypeId(ResultType, FloatType);
			TArray<TUniquePtr<SpvInstruction>> AcosValidationInsts;
			{
				SpvId One = GetScalarOrVectorId(ResultType, 1.0f);

				SpvId ExtSet450 = *Context.ExtSets.FindKey(SpvExtSet::GLSLstd450);
				SpvId FAbs = Patcher.NewId();
				auto FAbsOp = MakeUnique<SpvFAbs>(FloatResultType, ExtSet450, Inst->GetX());
				FAbsOp->SetId(FAbs);
				AcosValidationInsts.Add(MoveTemp(FAbsOp));

				SpvId FOrdGreaterThan = Patcher.NewId();
				auto FOrdGreaterThanOp = MakeUnique<SpvOpFOrdGreaterThan>(BoolResultType, FAbs, One);
				FOrdGreaterThanOp->SetId(FOrdGreaterThan);
				AcosValidationInsts.Add(MoveTemp(FOrdGreaterThanOp));

				SpvId Condition = FOrdGreaterThan;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, FOrdGreaterThan);
					AnyOp->SetId(Any);
					AcosValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				AcosValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				AcosValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				AcosValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(AcosValidationInsts);
				AcosValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				AcosValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(AcosValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvSqrt* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyComparisonZeroError([&] {return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetX(), SpvOp::FOrdLessThan);
		}
	}

	void SpvValidator::Visit(const SpvInverseSqrt* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyComparisonZeroError([&] {return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetX(), SpvOp::FOrdLessThanEqual);
		}
	}

	void SpvValidator::Visit(const SpvAtan2* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			TArray<TUniquePtr<SpvInstruction>> Atan2ValidationInsts;
			{
				SpvId Zero = GetScalarOrVectorId(ResultType, 0.0f);
				SpvId FOrdEqual1 = Patcher.NewId();
				{
					auto FOrdEqualOp = MakeUnique<SpvOpFOrdEqual>(BoolResultType, Inst->GetY(), Zero);
					FOrdEqualOp->SetId(FOrdEqual1);
					Atan2ValidationInsts.Add(MoveTemp(FOrdEqualOp));
				}
				SpvId FOrdEqual2 = Patcher.NewId();
				{
					auto FOrdEqualOp = MakeUnique<SpvOpFOrdEqual>(BoolResultType, Inst->GetX(), Zero);
					FOrdEqualOp->SetId(FOrdEqual2);
					Atan2ValidationInsts.Add(MoveTemp(FOrdEqualOp));
				}

				SpvId LogicalAnd = Patcher.NewId();
				auto LogicalAndOp = MakeUnique<SpvOpLogicalAnd>(BoolResultType, FOrdEqual1, FOrdEqual2);
				LogicalAndOp->SetId(LogicalAnd);
				Atan2ValidationInsts.Add(MoveTemp(LogicalAndOp));

				SpvId Condition = LogicalAnd;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, LogicalAnd);
					AnyOp->SetId(Any);
					Atan2ValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				Atan2ValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				Atan2ValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				Atan2ValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(Atan2ValidationInsts);
				Atan2ValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				Atan2ValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(Atan2ValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvFClamp* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			TArray<TUniquePtr<SpvInstruction>> ClampValidationInsts;
			{
				SpvId FOrdGreaterThan = Patcher.NewId();
				auto FOrdGreaterThanOp = MakeUnique<SpvOpFOrdGreaterThan>(BoolResultType, Inst->GetMinVal(), Inst->GetMaxVal());
				FOrdGreaterThanOp->SetId(FOrdGreaterThan);
				ClampValidationInsts.Add(MoveTemp(FOrdGreaterThanOp));

				SpvId Condition = FOrdGreaterThan;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, FOrdGreaterThan);
					AnyOp->SetId(Any);
					ClampValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ClampValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				ClampValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ClampValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ClampValidationInsts);
				ClampValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ClampValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(ClampValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvUClamp* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			TArray<TUniquePtr<SpvInstruction>> ClampValidationInsts;
			{
				SpvId UGreaterThan = Patcher.NewId();
				auto UGreaterThanOp = MakeUnique<SpvOpUGreaterThan>(BoolResultType, Inst->GetMinVal(), Inst->GetMaxVal());
				UGreaterThanOp->SetId(UGreaterThan);
				ClampValidationInsts.Add(MoveTemp(UGreaterThanOp));

				SpvId Condition = UGreaterThan;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, UGreaterThan);
					AnyOp->SetId(Any);
					ClampValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ClampValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				ClampValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ClampValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ClampValidationInsts);
				ClampValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ClampValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(ClampValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvSClamp* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			TArray<TUniquePtr<SpvInstruction>> ClampValidationInsts;
			{
				SpvId SGreaterThan = Patcher.NewId();
				auto SGreaterThanOp = MakeUnique<SpvOpSGreaterThan>(BoolResultType, Inst->GetMinVal(), Inst->GetMaxVal());
				SGreaterThanOp->SetId(SGreaterThan);
				ClampValidationInsts.Add(MoveTemp(SGreaterThanOp));

				SpvId Condition = SGreaterThan;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, SGreaterThan);
					AnyOp->SetId(Any);
					ClampValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ClampValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				ClampValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ClampValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ClampValidationInsts);
				ClampValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ClampValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(ClampValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvSmoothStep* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			TArray<TUniquePtr<SpvInstruction>> SmoothStepValidationInsts;
			{
				SpvId FOrdGreaterThanEqual = Patcher.NewId();
				auto FOrdGreaterThanEqualOp = MakeUnique<SpvOpFOrdGreaterThanEqual>(BoolResultType, Inst->GetEdge0(), Inst->GetEdge1());
				FOrdGreaterThanEqualOp->SetId(FOrdGreaterThanEqual);
				SmoothStepValidationInsts.Add(MoveTemp(FOrdGreaterThanEqualOp));

				SpvId Condition = FOrdGreaterThanEqual;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, FOrdGreaterThanEqual);
					AnyOp->SetId(Any);
					SmoothStepValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				SmoothStepValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				SmoothStepValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				SmoothStepValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(SmoothStepValidationInsts);
				SmoothStepValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				SmoothStepValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(SmoothStepValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvOpConvertFToU* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
			SpvId FloatInputTypeId = GetScalarOrVectorTypeId(ResultType, FloatType);
			SpvType* FloatInputType = Context.Types[FloatInputTypeId].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			TArray<TUniquePtr<SpvInstruction>> ConvertFValidationInsts;
			{
				SpvId Zero = GetScalarOrVectorId(FloatInputType, 0.0f);
				SpvId FOrdLessThan = Patcher.NewId();
				auto FOrdLessThanOp = MakeUnique<SpvOpFOrdLessThan>(BoolResultType, Inst->GetFloatValue(), Zero);
				FOrdLessThanOp->SetId(FOrdLessThan);
				ConvertFValidationInsts.Add(MoveTemp(FOrdLessThanOp));

				SpvId UpperBound = GetScalarOrVectorId(FloatInputType, 4294967296.0f);
				SpvId FOrdGTE = Patcher.NewId();
				auto FOrdGTEOp = MakeUnique<SpvOpFOrdGreaterThanEqual>(BoolResultType, Inst->GetFloatValue(), UpperBound);
				FOrdGTEOp->SetId(FOrdGTE);
				ConvertFValidationInsts.Add(MoveTemp(FOrdGTEOp));

				SpvId LogicalOr = Patcher.NewId();
				auto LogicalOrOp = MakeUnique<SpvOpLogicalOr>(BoolResultType, FOrdLessThan, FOrdGTE);
				LogicalOrOp->SetId(LogicalOr);
				ConvertFValidationInsts.Add(MoveTemp(LogicalOrOp));

				SpvId Condition = LogicalOr;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, LogicalOr);
					AnyOp->SetId(Any);
					ConvertFValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ConvertFValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				ConvertFValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ConvertFValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ConvertFValidationInsts);
				ConvertFValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ConvertFValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(ConvertFValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvOpConvertFToS* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
			SpvId FloatInputTypeId = GetScalarOrVectorTypeId(ResultType, FloatType);
			SpvType* FloatInputType = Context.Types[FloatInputTypeId].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			TArray<TUniquePtr<SpvInstruction>> ConvertFValidationInsts;
			{
				SpvId LowerBound = GetScalarOrVectorId(FloatInputType, -2147483648.0f);
				SpvId FOrdLT = Patcher.NewId();
				auto FOrdLTOp = MakeUnique<SpvOpFOrdLessThan>(BoolResultType, Inst->GetFloatValue(), LowerBound);
				FOrdLTOp->SetId(FOrdLT);
				ConvertFValidationInsts.Add(MoveTemp(FOrdLTOp));

				SpvId UpperBound = GetScalarOrVectorId(FloatInputType, 2147483648.0f);
				SpvId FOrdGTE = Patcher.NewId();
				auto FOrdGTEOp = MakeUnique<SpvOpFOrdGreaterThanEqual>(BoolResultType, Inst->GetFloatValue(), UpperBound);
				FOrdGTEOp->SetId(FOrdGTE);
				ConvertFValidationInsts.Add(MoveTemp(FOrdGTEOp));

				SpvId LogicalOr = Patcher.NewId();
				auto LogicalOrOp = MakeUnique<SpvOpLogicalOr>(BoolResultType, FOrdLT, FOrdGTE);
				LogicalOrOp->SetId(LogicalOr);
				ConvertFValidationInsts.Add(MoveTemp(LogicalOrOp));

				SpvId Condition = LogicalOr;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, LogicalOr);
					AnyOp->SetId(Any);
					ConvertFValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ConvertFValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				ConvertFValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ConvertFValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ConvertFValidationInsts);
				ConvertFValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ConvertFValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(ConvertFValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvOpUDiv* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyComparisonZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2(), SpvOp::IEqual);
		}
	}

	void SpvValidator::Visit(const SpvOpSDiv* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyComparisonZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2(), SpvOp::IEqual);
		}
	}

	void SpvValidator::Visit(const SpvOpFDiv* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyComparisonZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2(), SpvOp::FOrdEqual);
		}
	}

	void SpvValidator::Visit(const SpvOpUMod* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyComparisonZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2(), SpvOp::IEqual);
		}
	}

	void SpvValidator::Visit(const SpvOpSRem* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyComparisonZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2(), SpvOp::IEqual);

			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			TArray<TUniquePtr<SpvInstruction>> SRemValidationInsts;
			{
				SpvId IEqual1 = Patcher.NewId();
				auto IEqual1Op = MakeUnique<SpvOpIEqual>(BoolResultType, Inst->GetOperand1(), GetScalarOrVectorId(ResultType, std::numeric_limits<int32>::min()));
				IEqual1Op->SetId(IEqual1);
				SRemValidationInsts.Add(MoveTemp(IEqual1Op));

				SpvId IEqual2 = Patcher.NewId();
				auto IEqual2Op = MakeUnique<SpvOpIEqual>(BoolResultType, Inst->GetOperand2(), GetScalarOrVectorId(ResultType, -1));
				IEqual2Op->SetId(IEqual2);
				SRemValidationInsts.Add(MoveTemp(IEqual2Op));

				SpvId LogicalAnd = Patcher.NewId();
				auto LogicalAndOp = MakeUnique<SpvOpLogicalAnd>(BoolResultType, IEqual1, IEqual2);
				LogicalAndOp->SetId(LogicalAnd);
				SRemValidationInsts.Add(MoveTemp(LogicalAndOp));

				SpvId Condition = LogicalAnd;
				if (!ResultType->IsScalar())
				{
					SpvId Any = Patcher.NewId();
					auto AnyOp = MakeUnique<SpvOpAny>(BoolType, LogicalAnd);
					AnyOp->SetId(Any);
					SRemValidationInsts.Add(MoveTemp(AnyOp));
					Condition = Any;
				}

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				SRemValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				SRemValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				SRemValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(SRemValidationInsts);
				SRemValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				SRemValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			AddBlockSplittingInstructions(Inst->GetWordOffset().value(), MoveTemp(SRemValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvOpFRem* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyComparisonZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2(), SpvOp::FOrdEqual);
		}
	}

	void SpvValidator::PatchGetAccessFunc(SpvType* Type)
	{
		if (GetAccessFuncIds.Contains(Type))
		{
			return;
		}
		int32 MaxIndexNum = GetMaxIndexNum(Type);
		SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
		SpvId GetAccessFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(GetAccessFuncId, FString::Printf(TEXT("_GetAccess_%d_"), Type->GetId().GetValue())));
		TArray<TUniquePtr<SpvInstruction>> GetAccessFuncInsts;
		{
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());

			TArray<SpvId> ParamTypes;
			for (int32 i = 0; i < MaxIndexNum; i++)
			{
				ParamTypes.Add(IntType);
			}
			
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(IntType, ParamTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(IntType, SpvFunctionControl::DontInline, FuncType);
			FuncOp->SetId(GetAccessFuncId);
			GetAccessFuncInsts.Add(MoveTemp(FuncOp));

			TArray<SpvId> IndexParams;
			for (int32 i = 0; i < MaxIndexNum; i++)
			{
				SpvId IndexParam = Patcher.NewId();
				auto IndexParamOp = MakeUnique<SpvOpFunctionParameter>(IntType);
				IndexParamOp->SetId(IndexParam);
				GetAccessFuncInsts.Add(MoveTemp(IndexParamOp));
				Patcher.AddDebugName(MakeUnique<SpvOpName>(IndexParam, FString::Printf(TEXT("_Index%d_"), i)));
				IndexParams.Add(IndexParam);
			}

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			GetAccessFuncInsts.Add(MoveTemp(LabelOp));

			if (MaxIndexNum > 0)
			{
				SpvId FirstIndex = IndexParams[0];

				if (Type->GetKind() == SpvTypeKind::Vector)
				{
					SpvVectorType* VectorType = static_cast<SpvVectorType*>(Type);
					for (int Index = 0; Index < (int)VectorType->ElementCount; Index++)
					{
						SpvId IEqual = Patcher.NewId();
						auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolType, FirstIndex, Patcher.FindOrAddConstant(Index));
						IEqualOp->SetId(IEqual);
						GetAccessFuncInsts.Add(MoveTemp(IEqualOp));

						auto TrueLabel = Patcher.NewId();
						auto FalseLabel = Patcher.NewId();
						GetAccessFuncInsts.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
						GetAccessFuncInsts.Add(MakeUnique<SpvOpBranchConditional>(IEqual, TrueLabel, FalseLabel));

						auto TrueLabelOp = MakeUnique<SpvOpLabel>();
						TrueLabelOp->SetId(TrueLabel);
						GetAccessFuncInsts.Add(MoveTemp(TrueLabelOp));
						GetAccessFuncInsts.Add(MakeUnique<SpvOpReturnValue>(Patcher.FindOrAddConstant(Index * GetTypeByteSize(VectorType->ElementType) / 4)));

						auto FalseLabelOp = MakeUnique<SpvOpLabel>();
						FalseLabelOp->SetId(FalseLabel);
						GetAccessFuncInsts.Add(MoveTemp(FalseLabelOp));
					}
					AppendError(GetAccessFuncInsts);
					GetAccessFuncInsts.Add(MakeUnique<SpvOpReturnValue>(Patcher.FindOrAddConstant(0)));
				}
				else if (Type->GetKind() == SpvTypeKind::Struct)
				{
					SpvStructType* StructType = static_cast<SpvStructType*>(Type);
					int32 Offset = 0;
					for (int32 Index = 0; Index < StructType->MemberTypes.Num(); Index++)
					{
						SpvType* MemberType = StructType->MemberTypes[Index];

						SpvId IEqual = Patcher.NewId();
						auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolType, FirstIndex, Patcher.FindOrAddConstant(Index));
						IEqualOp->SetId(IEqual);
						GetAccessFuncInsts.Add(MoveTemp(IEqualOp));

						auto TrueLabel = Patcher.NewId();
						auto FalseLabel = Patcher.NewId();
						GetAccessFuncInsts.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
						GetAccessFuncInsts.Add(MakeUnique<SpvOpBranchConditional>(IEqual, TrueLabel, FalseLabel));

						auto TrueLabelOp = MakeUnique<SpvOpLabel>();
						TrueLabelOp->SetId(TrueLabel);
						GetAccessFuncInsts.Add(MoveTemp(TrueLabelOp));

						TArray<SpvId> MemberIndexes;
						int32 MemberMaxIndexNum = GetMaxIndexNum(MemberType);
						for (int i = 0; i < MemberMaxIndexNum; i++)
						{
							MemberIndexes.Add(IndexParams[i + 1]);
						}
						SpvId Ret = GetAccess(MemberType, MemberIndexes, GetAccessFuncInsts);

						SpvId AddRet = Patcher.NewId();
						auto AddOp = MakeUnique<SpvOpIAdd>(IntType, Ret, Patcher.FindOrAddConstant(Offset / 4));
						AddOp->SetId(AddRet);
						GetAccessFuncInsts.Add(MoveTemp(AddOp));

						GetAccessFuncInsts.Add(MakeUnique<SpvOpReturnValue>(AddRet));

						auto FalseLabelOp = MakeUnique<SpvOpLabel>();
						FalseLabelOp->SetId(FalseLabel);
						GetAccessFuncInsts.Add(MoveTemp(FalseLabelOp));

						Offset += GetTypeByteSize(MemberType);
					}
					AppendError(GetAccessFuncInsts);
					GetAccessFuncInsts.Add(MakeUnique<SpvOpReturnValue>(Patcher.FindOrAddConstant(0)));
				}
				else if (Type->GetKind() == SpvTypeKind::Array)
				{
					SpvArrayType* ArrayType = static_cast<SpvArrayType*>(Type);
					for (int Index = 0; Index < (int)ArrayType->Length; Index++)
					{
						SpvId IEqual = Patcher.NewId();
						auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolType, FirstIndex, Patcher.FindOrAddConstant(Index));
						IEqualOp->SetId(IEqual);
						GetAccessFuncInsts.Add(MoveTemp(IEqualOp));

						auto TrueLabel = Patcher.NewId();
						auto FalseLabel = Patcher.NewId();
						GetAccessFuncInsts.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
						GetAccessFuncInsts.Add(MakeUnique<SpvOpBranchConditional>(IEqual, TrueLabel, FalseLabel));

						auto TrueLabelOp = MakeUnique<SpvOpLabel>();
						TrueLabelOp->SetId(TrueLabel);
						GetAccessFuncInsts.Add(MoveTemp(TrueLabelOp));

						TArray<SpvId> MemberIndexes;
						int32 MemberMaxIndexNum = GetMaxIndexNum(ArrayType->ElementType);
						for (int i = 0; i < MemberMaxIndexNum; i++)
						{
							MemberIndexes.Add(IndexParams[i + 1]);
						}
						SpvId Ret = GetAccess(ArrayType->ElementType, MemberIndexes, GetAccessFuncInsts);

						SpvId AddRet = Patcher.NewId();
						auto AddOp = MakeUnique<SpvOpIAdd>(IntType, Ret, Patcher.FindOrAddConstant(Index * GetTypeByteSize(ArrayType->ElementType) / 4));
						AddOp->SetId(AddRet);
						GetAccessFuncInsts.Add(MoveTemp(AddOp));

						GetAccessFuncInsts.Add(MakeUnique<SpvOpReturnValue>(AddRet));

						auto FalseLabelOp = MakeUnique<SpvOpLabel>();
						FalseLabelOp->SetId(FalseLabel);
						GetAccessFuncInsts.Add(MoveTemp(FalseLabelOp));
					}
					AppendError(GetAccessFuncInsts);
					GetAccessFuncInsts.Add(MakeUnique<SpvOpReturnValue>(Patcher.FindOrAddConstant(0)));
				}
				else if (Type->GetKind() == SpvTypeKind::Matrix)
				{
					SpvMatrixType* MatrixType = static_cast<SpvMatrixType*>(Type);
					for (int Index = 0; Index < (int)MatrixType->ElementCount; Index++)
					{
						SpvId IEqual = Patcher.NewId();
						auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolType, FirstIndex, Patcher.FindOrAddConstant(Index));
						IEqualOp->SetId(IEqual);
						GetAccessFuncInsts.Add(MoveTemp(IEqualOp));

						auto TrueLabel = Patcher.NewId();
						auto FalseLabel = Patcher.NewId();
						GetAccessFuncInsts.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
						GetAccessFuncInsts.Add(MakeUnique<SpvOpBranchConditional>(IEqual, TrueLabel, FalseLabel));

						auto TrueLabelOp = MakeUnique<SpvOpLabel>();
						TrueLabelOp->SetId(TrueLabel);
						GetAccessFuncInsts.Add(MoveTemp(TrueLabelOp));

						TArray<SpvId> MemberIndexes;
						int32 MemberMaxIndexNum = GetMaxIndexNum(MatrixType->ElementType);
						for (int i = 0; i < MemberMaxIndexNum; i++)
						{
							MemberIndexes.Add(IndexParams[i + 1]);
						}
						SpvId Ret = GetAccess(MatrixType->ElementType, MemberIndexes, GetAccessFuncInsts);

						SpvId AddRet = Patcher.NewId();
						auto AddOp = MakeUnique<SpvOpIAdd>(IntType, Ret, Patcher.FindOrAddConstant(Index * GetTypeByteSize(MatrixType->ElementType) / 4));
						AddOp->SetId(AddRet);
						GetAccessFuncInsts.Add(MoveTemp(AddOp));

						GetAccessFuncInsts.Add(MakeUnique<SpvOpReturnValue>(AddRet));

						auto FalseLabelOp = MakeUnique<SpvOpLabel>();
						FalseLabelOp->SetId(FalseLabel);
						GetAccessFuncInsts.Add(MoveTemp(FalseLabelOp));
					}
					AppendError(GetAccessFuncInsts);
					GetAccessFuncInsts.Add(MakeUnique<SpvOpReturnValue>(Patcher.FindOrAddConstant(0)));
				}
			}
			else
			{
				GetAccessFuncInsts.Add(MakeUnique<SpvOpReturnValue>(Patcher.FindOrAddConstant(0)));
			}

			GetAccessFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(GetAccessFuncInsts));
		GetAccessFuncIds.Add(Type, GetAccessFuncId);
	}

	void SpvValidator::PatchAppendErrorFunc()
	{
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UInt2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 2));
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
		SpvId Float2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 2));
		SpvId Float4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 4));

		AppendErrorFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendErrorFuncId, "_AppendError_"));
		TArray<TUniquePtr<SpvInstruction>> AppendErrorFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::DontInline, FuncType);
			FuncOp->SetId(AppendErrorFuncId);
			AppendErrorFuncInsts.Add(MoveTemp(FuncOp));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendErrorFuncInsts.Add(MoveTemp(LabelOp));

			SpvId LoadedHasError = Patcher.NewId();
			auto LoadedHasErrorOp = MakeUnique<SpvOpLoad>(UIntType, HasError);
			LoadedHasErrorOp->SetId(LoadedHasError);
			AppendErrorFuncInsts.Add(MoveTemp(LoadedHasErrorOp));

			SpvId IEqual = Patcher.NewId();;
			auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolType, LoadedHasError, Patcher.FindOrAddConstant(0u));
			IEqualOp->SetId(IEqual);
			AppendErrorFuncInsts.Add(MoveTemp(IEqualOp));

			auto TrueLabel = Patcher.NewId();
			auto FalseLabel = Patcher.NewId();
			AppendErrorFuncInsts.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
			AppendErrorFuncInsts.Add(MakeUnique<SpvOpBranchConditional>(IEqual, TrueLabel, FalseLabel));

			auto FalseLabelOp = MakeUnique<SpvOpLabel>();
			FalseLabelOp->SetId(FalseLabel);
			AppendErrorFuncInsts.Add(MoveTemp(FalseLabelOp));
			AppendErrorFuncInsts.Add(MakeUnique<SpvOpReturn>());
			auto TrueLabelOp = MakeUnique<SpvOpLabel>();
			TrueLabelOp->SetId(TrueLabel);
			AppendErrorFuncInsts.Add(MoveTemp(TrueLabelOp));

			AppendErrorFuncInsts.Add(MakeUnique<SpvOpStore>(HasError, Patcher.FindOrAddConstant(1u)));

			{
				SpvId UVec4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 4));
				SpvId UVec4PointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UVec4Type));
				SpvId UIntPointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UIntType));
				SpvId Zero = Patcher.FindOrAddConstant(0u);

				// Access the first component of the first uvec4 element for atomic counter
				// AccessChain: DebuggerBuffer -> member 0 (RuntimeArray) -> element 0 (first uvec4) -> component 0 (first uint)
				SpvId DebuggerBufferStorage = Patcher.NewId();
				auto DebuggerBufferStorageOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, Zero, Zero});
				DebuggerBufferStorageOp->SetId(DebuggerBufferStorage);
				AppendErrorFuncInsts.Add(MoveTemp(DebuggerBufferStorageOp));

				// Atomic add 8 bytes (2 uints) to get a byte offset for storing FragCoord
				SpvId OriginalValue = Patcher.NewId();
				auto AtomicOp = MakeUnique<SpvOpAtomicIAdd>(UIntType, DebuggerBufferStorage, Patcher.FindOrAddConstant((uint32)SpvMemoryScope::Device),
					Patcher.FindOrAddConstant((uint32)SpvMemorySemantics::None), Patcher.FindOrAddConstant(8u));
				AtomicOp->SetId(OriginalValue);
				AppendErrorFuncInsts.Add(MoveTemp(AtomicOp));

				// Compute byte offsets for the two stores
				SpvId OriginalValuePlus4 = Patcher.NewId();
				auto Add4Op = MakeUnique<SpvOpIAdd>(UIntType, OriginalValue, Patcher.FindOrAddConstant(4u));
				Add4Op->SetId(OriginalValuePlus4);
				AppendErrorFuncInsts.Add(MoveTemp(Add4Op));

				SpvId OriginalValuePlus8 = Patcher.NewId();
				auto Add8Op = MakeUnique<SpvOpIAdd>(UIntType, OriginalValue, Patcher.FindOrAddConstant(8u));
				Add8Op->SetId(OriginalValuePlus8);
				AppendErrorFuncInsts.Add(MoveTemp(Add8Op));

				SpvId OriginalValuePlus12 = Patcher.NewId();
				auto Add12Op = MakeUnique<SpvOpIAdd>(UIntType, OriginalValue, Patcher.FindOrAddConstant(12u));
				Add12Op->SetId(OriginalValuePlus12);
				AppendErrorFuncInsts.Add(MoveTemp(Add12Op));

				// Bounds check: if OriginalValuePlus12 > 1024, return
				SpvId UGreaterThan = Patcher.NewId();;
				auto UGreaterThanOp = MakeUnique<SpvOpUGreaterThan>(BoolType, OriginalValuePlus12, Patcher.FindOrAddConstant(1024u));
				UGreaterThanOp->SetId(UGreaterThan);
				AppendErrorFuncInsts.Add(MoveTemp(UGreaterThanOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				AppendErrorFuncInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				AppendErrorFuncInsts.Add(MakeUnique<SpvOpBranchConditional>(UGreaterThan, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				AppendErrorFuncInsts.Add(MoveTemp(TrueLabelOp));
				AppendErrorFuncInsts.Add(MakeUnique<SpvOpReturn>());

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				AppendErrorFuncInsts.Add(MoveTemp(FalseLabelOp));

				{
					// For uvec4 buffer: byteOffset -> vecIndex = byteOffset / 16, componentIndex = (byteOffset / 4) % 4
					// byteOffset / 4 = ShiftRight by 2
					// byteOffset / 16 = ShiftRight by 4
					// componentIndex = (byteOffset / 4) & 3

					// Compute vec index and component index for OriginalValuePlus4 (FragCoord.x)
					SpvId DivBy4_X = Patcher.NewId();
					auto DivBy4_XOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, OriginalValuePlus4, Patcher.FindOrAddConstant(2u));
					DivBy4_XOp->SetId(DivBy4_X);
					AppendErrorFuncInsts.Add(MoveTemp(DivBy4_XOp));

					SpvId VecIndex_X = Patcher.NewId();
					auto VecIndex_XOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, OriginalValuePlus4, Patcher.FindOrAddConstant(4u));
					VecIndex_XOp->SetId(VecIndex_X);
					AppendErrorFuncInsts.Add(MoveTemp(VecIndex_XOp));

					SpvId CompIndex_X = Patcher.NewId();
					auto CompIndex_XOp = MakeUnique<SpvOpBitwiseAnd>(UIntType, DivBy4_X, Patcher.FindOrAddConstant(3u));
					CompIndex_XOp->SetId(CompIndex_X);
					AppendErrorFuncInsts.Add(MoveTemp(CompIndex_XOp));

					// Compute vec index and component index for OriginalValuePlus8 (FragCoord.y)
					SpvId DivBy4_Y = Patcher.NewId();
					auto DivBy4_YOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, OriginalValuePlus8, Patcher.FindOrAddConstant(2u));
					DivBy4_YOp->SetId(DivBy4_Y);
					AppendErrorFuncInsts.Add(MoveTemp(DivBy4_YOp));

					SpvId VecIndex_Y = Patcher.NewId();
					auto VecIndex_YOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, OriginalValuePlus8, Patcher.FindOrAddConstant(4u));
					VecIndex_YOp->SetId(VecIndex_Y);
					AppendErrorFuncInsts.Add(MoveTemp(VecIndex_YOp));

					SpvId CompIndex_Y = Patcher.NewId();
					auto CompIndex_YOp = MakeUnique<SpvOpBitwiseAnd>(UIntType, DivBy4_Y, Patcher.FindOrAddConstant(3u));
					CompIndex_YOp->SetId(CompIndex_Y);
					AppendErrorFuncInsts.Add(MoveTemp(CompIndex_YOp));

					// AccessChain to get pointer to the uint component: DebuggerBuffer[0][vecIndex][compIndex]
					SpvId DebuggerBufferStorageX = Patcher.NewId();
					auto DebuggerBufferStorageXOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, VecIndex_X, CompIndex_X});
					DebuggerBufferStorageXOp->SetId(DebuggerBufferStorageX);
					AppendErrorFuncInsts.Add(MoveTemp(DebuggerBufferStorageXOp));

					SpvId DebuggerBufferStorageY = Patcher.NewId();
					auto DebuggerBufferStorageYOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, VecIndex_Y, CompIndex_Y});
					DebuggerBufferStorageYOp->SetId(DebuggerBufferStorageY);
					AppendErrorFuncInsts.Add(MoveTemp(DebuggerBufferStorageYOp));

					SpvId LoadedFragCoord = Patcher.NewId();
					auto LoadedFragCoordOp = MakeUnique<SpvOpLoad>(Float4Type, Context.BuiltIns[SpvBuiltIn::FragCoord]);
					LoadedFragCoordOp->SetId(LoadedFragCoord);
					AppendErrorFuncInsts.Add(MoveTemp(LoadedFragCoordOp));

					SpvId FragCoordXY = Patcher.NewId();
					auto FragCoordXYOp = MakeUnique<SpvOpVectorShuffle>(Float2Type, LoadedFragCoord, LoadedFragCoord, TArray<uint32>{0, 1});
					FragCoordXYOp->SetId(FragCoordXY);
					AppendErrorFuncInsts.Add(MoveTemp(FragCoordXYOp));

					SpvId UIntFragCoordXY = Patcher.NewId();
					auto UIntFragCoordXYOp = MakeUnique<SpvOpConvertFToU>(UInt2Type, FragCoordXY);
					UIntFragCoordXYOp->SetId(UIntFragCoordXY);
					AppendErrorFuncInsts.Add(MoveTemp(UIntFragCoordXYOp));

					SpvId UIntFragCoordX = Patcher.NewId();
					auto ExtractXOp = MakeUnique<SpvOpCompositeExtract>(UIntType, UIntFragCoordXY, TArray<uint32>{0});
					ExtractXOp->SetId(UIntFragCoordX);
					AppendErrorFuncInsts.Add(MoveTemp(ExtractXOp));

					SpvId UIntFragCoordY= Patcher.NewId();
					auto ExtractYOp = MakeUnique<SpvOpCompositeExtract>(UIntType, UIntFragCoordXY, TArray<uint32>{1});
					ExtractYOp->SetId(UIntFragCoordY);
					AppendErrorFuncInsts.Add(MoveTemp(ExtractYOp));

					AppendErrorFuncInsts.Add(MakeUnique<SpvOpStore>(DebuggerBufferStorageX, UIntFragCoordX));
					AppendErrorFuncInsts.Add(MakeUnique<SpvOpStore>(DebuggerBufferStorageY, UIntFragCoordY));
				}
			}

			AppendErrorFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendErrorFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendErrorFuncInsts));
	}

	void SpvValidator::AppendError(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendErrorFuncId);
		FuncCallOp->SetId(Patcher.NewId());
		InstList.Add(MoveTemp(FuncCallOp));
	}

	int32 SpvValidator::ComputeConstantAccess(SpvType* Type, const TArray<SpvId>& Indexes, int32 StartIdx)
	{
		if (StartIdx >= Indexes.Num())
		{
			return 0;
		}

		int32 IndexVal = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Indexes[StartIdx]].Storage).Value.GetData();

		switch (Type->GetKind())
		{
		case SpvTypeKind::Vector:
		{
			SpvVectorType* VecType = static_cast<SpvVectorType*>(Type);
			return IndexVal * GetTypeByteSize(VecType->ElementType) / 4;
		}
		case SpvTypeKind::Array:
		{
			SpvArrayType* ArrType = static_cast<SpvArrayType*>(Type);
			return IndexVal * GetTypeByteSize(ArrType->ElementType) / 4 + ComputeConstantAccess(ArrType->ElementType, Indexes, StartIdx + 1);
		}
		case SpvTypeKind::Struct:
		{
			SpvStructType* StructType = static_cast<SpvStructType*>(Type);
			int32 Offset = 0;
			for (int32 i = 0; i < IndexVal; i++)
			{
				Offset += GetTypeByteSize(StructType->MemberTypes[i]);
			}
			return Offset / 4 + ComputeConstantAccess(StructType->MemberTypes[IndexVal], Indexes, StartIdx + 1);
		}
		case SpvTypeKind::Matrix:
		{
			SpvMatrixType* MatType = static_cast<SpvMatrixType*>(Type);
			return IndexVal * GetTypeByteSize(MatType->ElementType) / 4 + ComputeConstantAccess(MatType->ElementType, Indexes, StartIdx + 1);
		}
		default:
			return 0;
		}
	}

	SpvId SpvValidator::GetAccess(SpvType* VarType, const TArray<SpvId>& Indexes, TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		bool AllConstant = true;
		for (SpvId Index : Indexes)
		{
			if (!Context.Constants.contains(Index))
			{
				AllConstant = false;
				break;
			}
		}
		if (AllConstant)
		{
			return Patcher.FindOrAddConstant(ComputeConstantAccess(VarType, Indexes, 0));
		}

		int32 MaxIndexNum = GetMaxIndexNum(VarType);
		PatchGetAccessFunc(VarType);

		SpvId GetAccessFuncId = GetAccessFuncIds[VarType];
		SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));

		TArray<SpvId> Arguments;
		for (SpvId Index : Indexes)
		{
			SpvId BitCastValue = Patcher.NewId();
			auto BitCastOp = MakeUnique<SpvOpBitcast>(IntType, Index);
			BitCastOp->SetId(BitCastValue);
			InstList.Add(MoveTemp(BitCastOp));
			Arguments.Add(BitCastValue);
		}
		while (MaxIndexNum - Arguments.Num() > 0)
		{
			Arguments.Add(Patcher.FindOrAddConstant(0));
		}

		SpvId FuncCallRet = Patcher.NewId();
		auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(IntType, GetAccessFuncId, Arguments);
		FuncCallOp->SetId(FuncCallRet);
		InstList.Add(MoveTemp(FuncCallOp));
		
		return FuncCallRet;
	}

	void SpvValidator::AppendAnyComparisonZeroError(const TFunction<int32()>& OffsetEval, SpvId ResultTypeId, SpvId ValueId, SpvOp Comparison)
	{
		SpvType* ResultType = Context.Types[ResultTypeId].Get();
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
		TArray<TUniquePtr<SpvInstruction>> InstList;
		{
			SpvType* ScalarType;
			if (SpvVectorType* VecType = dynamic_cast<SpvVectorType*>(ResultType))
			{
				ScalarType = VecType->ElementType;
			}
			else
			{
				ScalarType = ResultType;
			}

			SpvId Zero;
			if (ScalarType->GetKind() == SpvTypeKind::Float)
			{
				Zero = GetScalarOrVectorId(ResultType, 0.0f);
			}
			else if (SpvIntegerType* IntegerType = dynamic_cast<SpvIntegerType*>(ScalarType))
			{
				if (IntegerType->IsSigend())
				{
					Zero = GetScalarOrVectorId(ResultType, 0);
				}
				else
				{
					Zero = GetScalarOrVectorId(ResultType, 0u);
				}
			}

			SpvId ComparisonId = Patcher.NewId();
			if (Comparison == SpvOp::IEqual)
			{
				auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolResultType, ValueId, Zero);
				IEqualOp->SetId(ComparisonId);
				InstList.Add(MoveTemp(IEqualOp));
			}
			else if (Comparison == SpvOp::FOrdEqual)
			{
				auto FOrdEqualOp = MakeUnique<SpvOpFOrdEqual>(BoolResultType, ValueId, Zero);
				FOrdEqualOp->SetId(ComparisonId);
				InstList.Add(MoveTemp(FOrdEqualOp));
			}
			else if (Comparison == SpvOp::FOrdLessThanEqual)
			{
				auto FOrdLessThanEqualOp = MakeUnique<SpvOpFOrdLessThanEqual>(BoolResultType, ValueId, Zero);
				FOrdLessThanEqualOp->SetId(ComparisonId);
				InstList.Add(MoveTemp(FOrdLessThanEqualOp));
			}
			else if (Comparison == SpvOp::FOrdLessThan)
			{
				auto FOrdLessThanOp = MakeUnique<SpvOpFOrdLessThan>(BoolResultType, ValueId, Zero);
				FOrdLessThanOp->SetId(ComparisonId);
				InstList.Add(MoveTemp(FOrdLessThanOp));
			}
			else
			{
				AUX::Unreachable();
			}

			SpvId Condition = ComparisonId;
			if (!ResultType->IsScalar())
			{
				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, ComparisonId);
				AnyOp->SetId(Any);
				InstList.Add(MoveTemp(AnyOp));
				Condition = Any;
			}

			auto TrueLabel = Patcher.NewId();
			auto FalseLabel = Patcher.NewId();
			InstList.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
			InstList.Add(MakeUnique<SpvOpBranchConditional>(Condition, TrueLabel, FalseLabel));

			auto TrueLabelOp = MakeUnique<SpvOpLabel>();
			TrueLabelOp->SetId(TrueLabel);
			InstList.Add(MoveTemp(TrueLabelOp));
			AppendError(InstList);
			InstList.Add(MakeUnique<SpvOpBranch>(FalseLabel));

			auto FalseLabelOp = MakeUnique<SpvOpLabel>();
			FalseLabelOp->SetId(FalseLabel);
			InstList.Add(MoveTemp(FalseLabelOp));
		}
		AddBlockSplittingInstructions(OffsetEval(), MoveTemp(InstList));
	}
}
