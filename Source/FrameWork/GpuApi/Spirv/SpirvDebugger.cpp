#include "CommonHeader.h"
#include "SpirvDebugger.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	int32 GetByteOffset(const SpvVariable* Var, const TArray<uint32>& Indexes)
	{
		int32 Offset = 0;
		SpvType* CurType = Var->Type;
		for (uint32 Index : Indexes)
		{
			if (CurType->GetKind() == SpvTypeKind::Vector)
			{
				SpvVectorType* VectorType = static_cast<SpvVectorType*>(CurType);
				Offset += Index * GetTypeByteSize(VectorType->ElementType);
			}
			else if (CurType->GetKind() == SpvTypeKind::Struct)
			{
				SpvStructType* StructType = static_cast<SpvStructType*>(CurType);
				for (uint32 MemberIndex = 0; MemberIndex < Index; MemberIndex++)
				{
					Offset += GetTypeByteSize(StructType->MemberTypes[MemberIndex]);
				}
				SpvType* MemberType = StructType->MemberTypes[Index];
				CurType = MemberType;
			}
			else if (CurType->GetKind() == SpvTypeKind::Array)
			{
				SpvArrayType* ArrayType = static_cast<SpvArrayType*>(CurType);
				Offset += Index * GetTypeByteSize(ArrayType->ElementType);
				CurType = ArrayType->ElementType;
			}
			else if (CurType->GetKind() == SpvTypeKind::Matrix)
			{
				SpvMatrixType* MatrixType = static_cast<SpvMatrixType*>(CurType);
				Offset += Index * GetTypeByteSize(MatrixType->ElementType);
				CurType = MatrixType->ElementType;
			}
			else
			{
				AUX::Unreachable();
			}
		}
		return Offset;
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
		SpvFunctionDesc* CurFuncDesc = static_cast<SpvFunctionDesc*>(Context.LexicalScopes[Inst->GetFunction()].Get());
		ShaderFunc* EditorFunc = Context.EditorFuncInfo.FindByPredicate([&](const ShaderFunc& InItem) {
			return InItem.FullName == CurFuncDesc->GetName() && InItem.Start.x == CurFuncDesc->GetLine();
		});
		for (int i = CurFuncParams.Num() - 1; i >= 0; i--)
		{
			if (EditorFunc->Params[i].SemaFlag != ParamSemaFlag::Out)
			{
				SpvPointer* Pointer = Context.FindPointer(CurFuncParams[i]->Id);
				SpvType* PointeeType = Pointer->Type->PointeeType;
				PatchAppendVarFunc(Pointer, 0);

				SpvId AppendVarFuncId = AppendVarFuncIds[{PointeeType, 0}];
				TArray<TUniquePtr<SpvInstruction>> AppendVarInsts;
				{
					SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::VarChange);
					SpvId Line = Patcher.FindOrAddConstant(0u);
					SpvId VarId = Patcher.FindOrAddConstant(Pointer->Var->Id.GetValue());
					SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());

					TArray<SpvId> Arguments{ StateType, Line, VarId };
					SpvId ParamValue = Patcher.NewId();
					auto ParamValueOp = MakeUnique<SpvOpLoad>(PointeeType->GetId(), Pointer->Id);
					ParamValueOp->SetId(ParamValue);
					AppendVarInsts.Add(MoveTemp(ParamValueOp));
					Arguments.Add(ParamValue);

					auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendVarFuncId, Arguments);
					FuncCallOp->SetId(Patcher.NewId());
					AppendVarInsts.Add(MoveTemp(FuncCallOp));
				}
				Patcher.AddInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(AppendVarInsts));
			}
		}
		CurFuncParams.Empty();
	}

	void SpvDebuggerVisitor::Visit(const SpvDebugLine* Inst)
	{
		CurLine = *(int32*)std::get<SpvObject::Internal>(Context.Constants[Inst->GetLineStart()].Storage).Value.GetData();
	}

	void SpvDebuggerVisitor::Visit(const SpvDebugScope* Inst)
	{
		CurScope = Context.LexicalScopes[Inst->GetScope()].Get();

		TArray<TUniquePtr<SpvInstruction>> AppendScopeInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::ScopeChange);
			SpvId ScopeId = Patcher.FindOrAddConstant(CurScope->GetId().GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendScopeFuncId, TArray<SpvId>{ StateType, ScopeId});
			FuncCallOp->SetId(Patcher.NewId());
			AppendScopeInsts.Add(MoveTemp(FuncCallOp));
		}

		int EndOffset = Inst->GetWordOffset().value() + Inst->GetWordLen().value();
		Patcher.AddInstructions(EndOffset, MoveTemp(AppendScopeInsts));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFunction* Inst)
	{
		CurReturnType = Context.Types[Inst->GetResultType()].Get();
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFunctionCall* Inst)
	{
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
		{
			TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::FuncCall);
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AppendTagInsts));
		}

		{
			TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::FuncCallAfterReturn);
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
			Patcher.AddInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(AppendTagInsts));
		}

		if(CurScope)
		{
			TArray<TUniquePtr<SpvInstruction>> AppendScopeInsts;
			{
				SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::ScopeChange);
				SpvId ScopeId = Patcher.FindOrAddConstant(CurScope->GetId().GetValue());
				SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
				auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendScopeFuncId, TArray<SpvId>{ StateType, ScopeId});
				FuncCallOp->SetId(Patcher.NewId());
				AppendScopeInsts.Add(MoveTemp(FuncCallOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(AppendScopeInsts));
		}
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFunctionParameter* Inst)
	{
		SpvId ResultId = Inst->GetId().value();

		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = { {ResultId, PointerType->PointeeType}, SpvStorageClass::Function};

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
		Context.FuncParameters.emplace(ResultId, &Context.LocalVariables[ResultId]);
		CurFuncParams.Add(&Context.LocalVariables[ResultId]);
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

	}

	void SpvDebuggerVisitor::Visit(const SpvOpLoad* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpStore* Inst)
	{
		SpvPointer* Pointer = Context.FindPointer(Inst->GetPointer());
		SpvType* PointeeType = Pointer->Type->PointeeType;
		uint32 IndexNum = Pointer->Indexes.Num();
		PatchAppendVarFunc(Pointer, IndexNum);

		SpvId AppendVarFuncId = AppendVarFuncIds[{PointeeType, IndexNum}];
		TArray<TUniquePtr<SpvInstruction>> AppendVarInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::VarChange);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VarId = Patcher.FindOrAddConstant(Pointer->Var->Id.GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> Arguments{ StateType, Line, VarId };
			if (IndexNum > 0)
			{
				SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
				SpvId ArrType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeArray>(UIntType, Patcher.FindOrAddConstant(IndexNum)));
				SpvId IndexArr = Patcher.NewId();
				TArray<SpvId> Constituents;
				for (SpvId Index : Pointer->Indexes)
				{
					SpvId BitCastValue = Patcher.NewId();
					auto BitCastOp = MakeUnique<SpvOpBitcast>(UIntType, Index);
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
			AppendVarInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(AppendVarInsts));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpAccessChain* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());

		SpvPointer Pointer{
			.Id = ResultId,
			.Var = Context.FindPointer(Inst->GetBasePointer())->Var,
			.Indexes = Inst->GetIndexes(),
			.Type = PointerType
		};
		Context.LocalPointers.emplace(ResultId, MoveTemp(Pointer));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpSwitch* Inst)
	{
		TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::Condition);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
		}
		int OpMergeOffset = (*Insts)[InstIndex - 1]->GetWordOffset().value();
		Patcher.AddInstructions(OpMergeOffset, MoveTemp(AppendTagInsts));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpConvertFToU* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpConvertFToS* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpUDiv* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpSDiv* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpUMod* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpSRem* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFRem* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvOpBranch* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvOpBranchConditional* Inst)
	{
		TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::Condition);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
		}
		int OpMergeOffset = (*Insts)[InstIndex - 1]->GetWordOffset().value();
		Patcher.AddInstructions(OpMergeOffset, MoveTemp(AppendTagInsts));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpReturn* Inst)
	{
		TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::Return);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AppendTagInsts));
	}

	void SpvDebuggerVisitor::Visit(const SpvOpReturnValue* Inst)
	{
		PatchAppendValueFunc(CurReturnType);
		SpvId AppendValueFuncId = AppendValueFuncIds[CurReturnType];
		TArray<TUniquePtr<SpvInstruction>> AppendValueInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::ReturnValue);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendValueFuncId, TArray<SpvId>{ StateType, Line, Inst->GetValue()});
			FuncCallOp->SetId(Patcher.NewId());
			AppendValueInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AppendValueInsts));

		TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::Return);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AppendTagInsts));
	}

	void SpvDebuggerVisitor::Visit(const SpvPow* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvFClamp* Inst)
	{

	}

	void SpvDebuggerVisitor::Visit(const SpvUClamp* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvSClamp* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvSmoothStep* Inst)
	{
		
	}

	void SpvDebuggerVisitor::Visit(const SpvNormalize* Inst)
	{
		
	}

	int32 SpvDebuggerVisitor::GetInstIndex(SpvId Inst) const
	{
		return Insts->IndexOfByPredicate([this, Inst](const TUniquePtr<SpvInstruction>& InInst){
			return InInst->GetId() ? InInst->GetId().value() == Inst : false;
		});
	}

	void SpvDebuggerVisitor::PatchToDebugger(SpvId InValueId, SpvId InTypeId, TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvType* Type = Context.Types.at(InTypeId).Get();
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		if (InTypeId == UIntType)
		{
			SpvId LoadedDebuggerOffset = Patcher.NewId();
			auto LoadedDebuggerOffsetOp = MakeUnique<SpvOpLoad>(UIntType, DebuggerOffset);
			LoadedDebuggerOffsetOp->SetId(LoadedDebuggerOffset);
			InstList.Add(MoveTemp(LoadedDebuggerOffsetOp));

			SpvId AlignedDebuggerOffset = Patcher.NewId();
			auto AlignedDebuggerOffsetOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant(2u));
			AlignedDebuggerOffsetOp->SetId(AlignedDebuggerOffset);
			InstList.Add(MoveTemp(AlignedDebuggerOffsetOp));

			SpvId UIntPointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UIntType));
			SpvId DebuggerBufferStorage = Patcher.NewId();
			auto DebuggerBufferStorageOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Patcher.FindOrAddConstant(0u), AlignedDebuggerOffset});
			DebuggerBufferStorageOp->SetId(DebuggerBufferStorage);
			InstList.Add(MoveTemp(DebuggerBufferStorageOp));

			InstList.Add(MakeUnique<SpvOpStore>(DebuggerBufferStorage, InValueId));

			SpvId NewDebuggerOffset = Patcher.NewId();
			auto AddOp = MakeUnique<SpvOpIAdd>(UIntType, LoadedDebuggerOffset, Patcher.FindOrAddConstant(4u));
			AddOp->SetId(NewDebuggerOffset);
			InstList.Add(MoveTemp(AddOp));

			InstList.Add(MakeUnique<SpvOpStore>(DebuggerOffset, NewDebuggerOffset));
		}
		else
		{
			if (Type->IsScalar())
			{
				if (Type->GetKind() == SpvTypeKind::Bool)
				{
					SpvId SelectValue = Patcher.NewId();
					auto SelectOp = MakeUnique<SpvOpSelect>(UIntType, InValueId, Patcher.FindOrAddConstant(1u), Patcher.FindOrAddConstant(0u));
					SelectOp->SetId(SelectValue);
					InstList.Add(MoveTemp(SelectOp));
					PatchToDebugger(SelectValue, UIntType, InstList);
				}
				else
				{
					SpvId BitCastValue = Patcher.NewId();
					auto BitCastOp = MakeUnique<SpvOpBitcast>(UIntType, InValueId);
					BitCastOp->SetId(BitCastValue);
					InstList.Add(MoveTemp(BitCastOp));
					PatchToDebugger(BitCastValue, UIntType, InstList);
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
					PatchToDebugger(ExtractedValue, ElementTypeId, InstList);
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
					PatchToDebugger(ExtractedValue, MemberTypeId, InstList);
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
					PatchToDebugger(ExtractedValue, ElementTypeId, InstList);
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
					PatchToDebugger(ExtractedValue, ElementTypeId, InstList);
				}
			}
		}

	}

	void SpvDebuggerVisitor::PatchAppendVarFunc(SpvPointer* Pointer, uint32 IndexNum)
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
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> ParamterTypes;
			ParamterTypes.Add(UIntType);
			ParamterTypes.Add(UIntType);
			ParamterTypes.Add(UIntType);

			SpvId ArrType;
			if (IndexNum > 0)
			{
				ArrType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeArray>(UIntType, Patcher.FindOrAddConstant(IndexNum)));
				ParamterTypes.Add(ArrType);
			}

			ParamterTypes.Add(PointeeType->GetId());

			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, ParamterTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendVarFuncId);
			AppendVarFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StateTypeParam = Patcher.NewId();
			auto StateTypeParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			StateTypeParamOp->SetId(StateTypeParam);
			AppendVarFuncInsts.Add(MoveTemp(StateTypeParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateTypeParam, "_DebuggerStateType_"));

			SpvId LineParam = Patcher.NewId();
			auto LineParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			LineParamOp->SetId(LineParam);
			AppendVarFuncInsts.Add(MoveTemp(LineParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(LineParam, "_DebuggerLine_"));

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

			PatchActiveCondition(AppendVarFuncInsts);
			PatchToDebugger(StateTypeParam, UIntType, AppendVarFuncInsts);
			PatchToDebugger(LineParam, UIntType, AppendVarFuncInsts);
			PatchToDebugger(VarIdParam, UIntType, AppendVarFuncInsts);
			PatchToDebugger(Patcher.FindOrAddConstant(IndexNum), UIntType, AppendVarFuncInsts);
			if (IndexNum > 0)
			{
				PatchToDebugger(IndexArrParam, ArrType, AppendVarFuncInsts);
			}
			PatchToDebugger(Patcher.FindOrAddConstant((uint32)GetTypeByteSize(PointeeType)), UIntType, AppendVarFuncInsts);
			PatchToDebugger(ValueParam, PointeeType->GetId(), AppendVarFuncInsts);

			AppendVarFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendVarFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendVarFuncInsts));
		AppendVarFuncIds.Add({ PointeeType, IndexNum }, AppendVarFuncId);
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
			ParamterTypes.Add(UIntType);
			ParamterTypes.Add(UIntType);
			ParamterTypes.Add(ValueType->GetId());

			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, ParamterTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendValueFuncId);
			AppendValueFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StateTypeParam = Patcher.NewId();
			auto StateTypeParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			StateTypeParamOp->SetId(StateTypeParam);
			AppendValueFuncInsts.Add(MoveTemp(StateTypeParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateTypeParam, "_DebuggerStateType_"));

			SpvId LineParam = Patcher.NewId();
			auto LineParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			LineParamOp->SetId(LineParam);
			AppendValueFuncInsts.Add(MoveTemp(LineParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(LineParam, "_DebuggerLine_"));

			SpvId ValueParam = Patcher.NewId();
			auto ValueParamOp = MakeUnique<SpvOpFunctionParameter>(ValueType->GetId());
			ValueParamOp->SetId(ValueParam);
			AppendValueFuncInsts.Add(MoveTemp(ValueParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(ValueParam, "_DebuggerValue_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendValueFuncInsts.Add(MoveTemp(LabelOp));

			PatchActiveCondition(AppendValueFuncInsts);
			PatchToDebugger(StateTypeParam, UIntType, AppendValueFuncInsts);
			PatchToDebugger(LineParam, UIntType, AppendValueFuncInsts);
			PatchToDebugger(Patcher.FindOrAddConstant((uint32)GetTypeByteSize(ValueType)), UIntType, AppendValueFuncInsts);
			PatchToDebugger(ValueParam, ValueType->GetId(), AppendValueFuncInsts);

			AppendValueFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendValueFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendValueFuncInsts));
		AppendValueFuncIds.Add(ValueType, AppendValueFuncId);
	}

	void SpvDebuggerVisitor::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections, const TMap<SpvId, SpvExtSet>& InExtSets)
	{
		this->Insts = &Insts;
		Patcher.SetSpvContext(Insts, SpvCode, &Context);
		//Get entry point loc
		InstIndex = GetInstIndex(Context.EntryPoint);

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

		//Patch debugger buffer
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
		SpvId RunTimeArrayType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeRuntimeArray>(UIntType));
		SpvId DebuggerBufferType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeStruct>(TArray<SpvId>{RunTimeArrayType}));
		SpvId DebuggerBufferPointerType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, DebuggerBufferType));

		int ArrayStride = 4;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(RunTimeArrayType, SpvDecorationKind::ArrayStride, TArray<uint8>{ (uint8*)&ArrayStride, sizeof(int) }));
		int MemberOffset = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpMemberDecorate>(DebuggerBufferType, 0, SpvDecorationKind::Offset, TArray<uint8>{ (uint8*)&MemberOffset, sizeof(int) }));
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBufferType, SpvDecorationKind::BufferBlock));

		DebuggerBuffer = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(DebuggerBufferPointerType, SpvStorageClass::Uniform);
			VarOp->SetId(DebuggerBuffer);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerBuffer, "_DebuggerBuffer_"));
		}
		int SetNumber = 0;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBuffer, SpvDecorationKind::DescriptorSet, TArray<uint8>{ (uint8*)&SetNumber, sizeof(int) }));
		int BindingNumber = 0721;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBuffer, SpvDecorationKind::Binding, TArray<uint8>{ (uint8*)&BindingNumber, sizeof(int) }));

		DebuggerOffset = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(DebuggerOffset);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(DebuggerOffset, "_DebuggerOffset_"));
		}

		//Patch AppendScope function
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		AppendScopeFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendScopeFuncId, "_AppendScope_"));
		TArray<TUniquePtr<SpvInstruction>> AppendScopeFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, TArray<SpvId>{ UIntType, UIntType }));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendScopeFuncId);
			AppendScopeFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StateTypeParam = Patcher.NewId();
			auto StateTypeParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			StateTypeParamOp->SetId(StateTypeParam);
			AppendScopeFuncInsts.Add(MoveTemp(StateTypeParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateTypeParam, "_DebuggerStateType_"));

			SpvId ScopeIdParam = Patcher.NewId();
			auto ScopeIdParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			ScopeIdParamOp->SetId(ScopeIdParam);
			AppendScopeFuncInsts.Add(MoveTemp(ScopeIdParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(ScopeIdParam, "_DebuggerScopeId_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendScopeFuncInsts.Add(MoveTemp(LabelOp));

			PatchActiveCondition(AppendScopeFuncInsts);
			PatchToDebugger(StateTypeParam, UIntType, AppendScopeFuncInsts);
			PatchToDebugger(ScopeIdParam, UIntType, AppendScopeFuncInsts);

			AppendScopeFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendScopeFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendScopeFuncInsts));

		AppendTagFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendTagFuncId, "_AppendTag_"));
		TArray<TUniquePtr<SpvInstruction>> AppendTagFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, TArray<SpvId>{ UIntType, UIntType }));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendTagFuncId);
			AppendTagFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StateTypeParam = Patcher.NewId();
			auto StateTypeParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			StateTypeParamOp->SetId(StateTypeParam);
			AppendTagFuncInsts.Add(MoveTemp(StateTypeParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateTypeParam, "_DebuggerStateType_"));

			SpvId LineParam = Patcher.NewId();
			auto LineParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			LineParamOp->SetId(LineParam);
			AppendTagFuncInsts.Add(MoveTemp(LineParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(LineParam, "_DebuggerLine_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendTagFuncInsts.Add(MoveTemp(LabelOp));

			PatchActiveCondition(AppendTagFuncInsts);
			PatchToDebugger(StateTypeParam, UIntType, AppendTagFuncInsts);
			PatchToDebugger(LineParam, UIntType, AppendTagFuncInsts);

			AppendTagFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendTagFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendTagFuncInsts));

		while (InstIndex < (*Insts).Num())
		{
			(*Insts)[InstIndex]->Accept(this);
			InstIndex++;
		}
	}
}
