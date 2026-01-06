#include "CommonHeader.h"
#include "SpirvDebugger.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
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
				if (Increment > GetTypeByteSize(VectorType) || Increment < 0)
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
				if (Increment > GetTypeByteSize(ArrayType) || Increment < 0)
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
				if (Increment > GetTypeByteSize(MatrixType) || Increment < 0)
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

	SpvId PatchDebuggerBuffer(SpvPatcher& Patcher)
	{
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId RunTimeArrayType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeRuntimeArray>(UIntType));
		SpvId DebuggerBufferType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeStruct>(TArray<SpvId>{RunTimeArrayType}));
		SpvId DebuggerBufferPointerType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, DebuggerBufferType));

		int ArrayStride = 4;
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
		int BindingNumber = 0721;
		Patcher.AddAnnotation(MakeUnique<SpvOpDecorate>(DebuggerBuffer, SpvDecorationKind::Binding, TArray<uint8>{ (uint8*)&BindingNumber, sizeof(int) }));

		return DebuggerBuffer;
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
	}

	void SpvDebuggerVisitor::Visit(const SpvDebugScope* Inst)
	{
		CurScope = Context.LexicalScopes[Inst->GetScope()].Get();
		AppendScope([&] { return Inst->GetWordOffset().value() + Inst->GetWordLen().value(); });
	}

	void SpvDebuggerVisitor::Visit(const SpvOpFunction* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		Context.Funcs.emplace(ResultId, SpvFunc{Inst->GetFunctionType(), Inst->GetResultType()});
		CurFunc = &Context.Funcs[ResultId];
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
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::FuncCall);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId CallId = Patcher.FindOrAddConstant(ResultId.GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendCallFuncId, TArray<SpvId>{ StateType, Line, CallId});
			FuncCallOp->SetId(Patcher.NewId());
			AppendCallInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AppendCallInsts));

		AppendTag([&] { return Inst->GetWordOffset().value() + Inst->GetWordLen().value(); }, SpvDebuggerStateType::FuncCallAfterReturn);
		AppendScope([&] { return Inst->GetWordOffset().value() + Inst->GetWordLen().value(); });
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
		CurFunc->Parameters.Add(ResultId);
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
		Context.BBs.emplace(ResultId, SpvBasicBlock{});
		CurBlock = &Context.BBs[ResultId];
	}

	void SpvDebuggerVisitor::Visit(const SpvOpLoad* Inst)
	{
		if (EnableUbsan)
		{
			SpvPointer* Pointer = Context.FindPointer(Inst->GetPointer());
			AppendAccess([&] {return Inst->GetWordOffset().value(); }, Pointer);
		}

	}

	void SpvDebuggerVisitor::Visit(const SpvOpStore* Inst)
	{
		SpvPointer* Pointer = Context.FindPointer(Inst->GetPointer());
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
		if (Language == GpuShaderLanguage::HLSL)
		{
			AppendTag([&] { return (*Insts)[InstIndex - 1]->GetWordOffset().value(); }, SpvDebuggerStateType::Condition);
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
		SpvType* Type = Context.Types.at(InTypeId).Get();
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
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
				else if(SpvIntegerType* IntegerType = dynamic_cast<SpvIntegerType*>(Type) ; IntegerType && IntegerType->GetWidth() == 64)
				{
					SpvId UInt2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 2));
					SpvId UInt64Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(64, 0));
					if (IntegerType->IsSigend())
					{
						SpvId UInt64Value = Patcher.NewId();
						{
							auto BitCastOp = MakeUnique<SpvOpBitcast>(UInt64Type, InValueId);
							BitCastOp->SetId(UInt64Value);
							InstList.Add(MoveTemp(BitCastOp));
						}
						SpvId BitCastValue = Patcher.NewId();
						auto BitCastOp = MakeUnique<SpvOpBitcast>(UInt2Type, UInt64Value);
						BitCastOp->SetId(BitCastValue);
						InstList.Add(MoveTemp(BitCastOp));
						PatchToDebugger(BitCastValue, UInt2Type, InstList);
					}
					else
					{
						SpvId BitCastValue = Patcher.NewId();
						auto BitCastOp = MakeUnique<SpvOpBitcast>(UInt2Type, InValueId);
						BitCastOp->SetId(BitCastValue);
						InstList.Add(MoveTemp(BitCastOp));
						PatchToDebugger(BitCastValue, UInt2Type, InstList);
					}
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
			ParamterTypes.Add(UIntType);
			ParamterTypes.Add(UIntType);
			ParamterTypes.Add(UIntType);
			if (IndexNum > 0)
			{
				ParamterTypes.Add(IntType);
			}

			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, ParamterTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendAccessFuncId);
			AppendAccessFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StateTypeParam = Patcher.NewId();
			auto StateTypeParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			StateTypeParamOp->SetId(StateTypeParam);
			AppendAccessFuncInsts.Add(MoveTemp(StateTypeParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateTypeParam, "_DebuggerStateType_"));

			SpvId LineParam = Patcher.NewId();
			auto LineParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			LineParamOp->SetId(LineParam);
			AppendAccessFuncInsts.Add(MoveTemp(LineParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(LineParam, "_DebuggerLine_"));

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

			PatchActiveCondition(AppendAccessFuncInsts);
			PatchToDebugger(StateTypeParam, UIntType, AppendAccessFuncInsts);
			PatchToDebugger(LineParam, UIntType, AppendAccessFuncInsts);
			PatchToDebugger(VarIdParam, UIntType, AppendAccessFuncInsts);
			PatchToDebugger(Patcher.FindOrAddConstant(IndexNum), UIntType, AppendAccessFuncInsts);
			for (int32 i = 0; i < IndexNum; i++)
			{
				PatchToDebugger(IndexParams[i], UIntType, AppendAccessFuncInsts);
			}

			AppendAccessFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendAccessFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendAccessFuncInsts));
		AppendAccessFuncIds.Add(IndexNum, AppendAccessFuncId);
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
			ParamterTypes.Add(UIntType);
			ParamterTypes.Add(UIntType);
			ParamterTypes.Add(UIntType);
			for (int32 i = 0; i < OperandNum; i++)
			{
				ParamterTypes.Add(OperandType->GetId());
			}

			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, ParamterTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendMathFuncId);
			AppendMathFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StateTypeParam = Patcher.NewId();
			auto StateTypeParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			StateTypeParamOp->SetId(StateTypeParam);
			AppendMathFuncInsts.Add(MoveTemp(StateTypeParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateTypeParam, "_DebuggerStateType_"));

			SpvId LineParam = Patcher.NewId();
			auto LineParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			LineParamOp->SetId(LineParam);
			AppendMathFuncInsts.Add(MoveTemp(LineParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(LineParam, "_DebuggerLine_"));

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

			PatchActiveCondition(AppendMathFuncInsts);
			PatchToDebugger(StateTypeParam, UIntType, AppendMathFuncInsts);
			PatchToDebugger(LineParam, UIntType, AppendMathFuncInsts);
			PatchToDebugger(ResultTypeIdParam, UIntType, AppendMathFuncInsts);
			for (int32 i = 0; i < OperandNum; i++)
			{
				PatchToDebugger(OperandParams[i], OperandType->GetId(), AppendMathFuncInsts);
			}

			AppendMathFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendMathFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendMathFuncInsts));
		AppendMathFuncIds.Add({ ResultType, OperandType, OperandNum }, AppendMathFuncId);
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
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::VarChange);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VarId = Patcher.FindOrAddConstant(Pointer->Var->Id.GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> Arguments{ StateType, Line, VarId };
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
		}
		Patcher.AddInstructions(OffsetEval(), MoveTemp(AppendVarInsts));
		return FuncCall;
	}

	void SpvDebuggerVisitor::AppendScope(const TFunction<int32()>& OffsetEval)
	{
		if (!CurScope)
		{
			return;
		}

		TArray<TUniquePtr<SpvInstruction>> AppendScopeInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::ScopeChange);
			SpvId ScopeId = Patcher.FindOrAddConstant(CurScope->GetId().GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendScopeFuncId, TArray<SpvId>{ StateType, ScopeId});
			FuncCallOp->SetId(Patcher.NewId());
			AppendScopeInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(OffsetEval(), MoveTemp(AppendScopeInsts));
	}

	void SpvDebuggerVisitor::AppendAccess(const TFunction<int32()>& OffsetEval, SpvPointer* Pointer)
	{
		int32 IndexNum = Pointer->Indexes.Num();
		PatchAppendAccessFunc(IndexNum);
		SpvId AppendAccessFuncId = AppendAccessFuncIds[IndexNum];
		TArray<TUniquePtr<SpvInstruction>> AppendAccessInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::Access);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VarId = Patcher.FindOrAddConstant(Pointer->Var->Id.GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> Arguments = { StateType, Line, VarId };
			Arguments.Append(Pointer->Indexes);
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
			ParamterTypes.Add(UIntType);
			ParamterTypes.Add(UIntType);
			ParamterTypes.Add(UIntType);

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

	void SpvDebuggerVisitor::AppendTag(const TFunction<int32()>& OffsetEval, SpvDebuggerStateType InStateType)
	{
		if (CurLine && CurBlock)
		{
			CurBlock->ValidLines.AddUnique(CurLine);
		}

		TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)InStateType);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
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
			SpvId StateType = Patcher.FindOrAddConstant((uint32)InStateType);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendValueFuncId, TArray<SpvId>{ StateType, Line, Value});
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
			SpvId StateType = Patcher.FindOrAddConstant((uint32)InStateType);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId ResultTypeId = Patcher.FindOrAddConstant(ResultType->GetId().GetValue());
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			TArray<SpvId> Arguments = { StateType, Line, ResultTypeId };
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

		AppendCallFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(AppendCallFuncId, "_AppendCall_"));
		TArray<TUniquePtr<SpvInstruction>> AppendCallFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, TArray<SpvId>{ UIntType, UIntType }));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(AppendCallFuncId);
			AppendCallFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StateTypeParam = Patcher.NewId();
			auto StateTypeParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			StateTypeParamOp->SetId(StateTypeParam);
			AppendCallFuncInsts.Add(MoveTemp(StateTypeParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StateTypeParam, "_DebuggerStateType_"));

			SpvId LineParam = Patcher.NewId();
			auto LineParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			LineParamOp->SetId(LineParam);
			AppendCallFuncInsts.Add(MoveTemp(LineParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(LineParam, "_DebuggerLine_"));

			SpvId CallIdParam = Patcher.NewId();
			auto CallIdParamOp = MakeUnique<SpvOpFunctionParameter>(UIntType);
			CallIdParamOp->SetId(CallIdParam);
			AppendCallFuncInsts.Add(MoveTemp(CallIdParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(CallIdParam, "_DebuggerCallId_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			AppendCallFuncInsts.Add(MoveTemp(LabelOp));

			PatchActiveCondition(AppendCallFuncInsts);
			PatchToDebugger(StateTypeParam, UIntType, AppendCallFuncInsts);
			PatchToDebugger(LineParam, UIntType, AppendCallFuncInsts);
			PatchToDebugger(CallIdParam, UIntType, AppendCallFuncInsts);

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
