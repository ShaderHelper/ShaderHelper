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
		if (VarSize > 0 && VarSize % 4 == 0)
		{
			SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
			SpvId ArrType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeArray>(UIntType, Patcher.FindOrAddConstant(VarSize / 4)));
			SpvId InitializedRange = Patcher.NewId();
			{
				SpvId VarPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, ArrType));
				auto VarOp = MakeUnique<SpvOpVariable>(VarPointerPrivateType, SpvStorageClass::Private);
				VarOp->SetId(InitializedRange);
				Patcher.AddGlobalVariable(MoveTemp(VarOp));
				FString VarName = FString::FromInt(Var->Id.GetValue());
				if (Context.Names.contains(Var->Id))
				{
					VarName = Context.Names[Var->Id];
				}
				Patcher.AddDebugName(MakeUnique<SpvOpName>(InitializedRange, FString::Printf(TEXT("_InitializedRange_%s_"), *VarName)));
			}
			VarInitializedRange.Add(Var->Id, InitializedRange);
		}
	}

	void SpvValidator::Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections, const TMap<SpvId, SpvExtSet>& InExtSets)
	{
		this->Insts = &Insts;
		Patcher.SetSpvContext(Insts, SpvCode, &Context);

		DebuggerBuffer = PatchDebuggerBuffer(Patcher);

		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
		HasError = Patcher.NewId();
		{
			auto VarOp = MakeUnique<SpvOpVariable>(UIntPointerPrivateType, SpvStorageClass::Private);
			VarOp->SetId(HasError);
			Patcher.AddGlobalVariable(MoveTemp(VarOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(HasError, "_HasError_"));
		}

		PatchAppendErrorFunc();
		
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
		if (EnableUbsan)
		{
			PatchVarInitializedRange(&LocalVariables[ResultId]);
		}
	}

	void SpvValidator::Visit(const SpvOpFunction* Inst)
	{
		SpvId ResultId = Inst->GetId().value();
		Funcs.Add(ResultId, SpvFunc{ Inst->GetFunctionType(), Inst->GetResultType() });
		CurFunc = &Funcs[ResultId];
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

				SpvVariable* ArgumentVar = &LocalVariables[Argument];
				int32 VarSize = GetTypeByteSize(ArgumentVar->Type);
				SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
				SpvId ArrType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeArray>(UIntType, Patcher.FindOrAddConstant(VarSize / 4)));

				{
					TArray<TUniquePtr<SpvInstruction>> InstList;
					SpvId LoadedArgument = Patcher.NewId();
					auto LoadedArgumentOp = MakeUnique<SpvOpLoad>(ArrType, VarInitializedRange[Argument]);
					LoadedArgumentOp->SetId(LoadedArgument);
					InstList.Add(MoveTemp(LoadedArgumentOp));

					InstList.Add(MakeUnique<SpvOpStore>(VarInitializedRange[Parameter], LoadedArgument));

					Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(InstList));
				}
			
				{
					TArray<TUniquePtr<SpvInstruction>> InstList;
					SpvId LoadedParameter = Patcher.NewId();
					auto LoadedParameterOp = MakeUnique<SpvOpLoad>(ArrType, VarInitializedRange[Parameter]);
					LoadedParameterOp->SetId(LoadedParameter);
					InstList.Add(MoveTemp(LoadedParameterOp));

					InstList.Add(MakeUnique<SpvOpStore>(VarInitializedRange[Argument], LoadedParameter));

					Patcher.AddInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(InstList));
				}
			}
		}
	}

	void SpvValidator::Visit(const SpvOpFunctionParameter* Inst)
	{
		SpvId ResultId = Inst->GetId().value();

		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvVariable Var = { {ResultId, PointerType->PointeeType}, SpvStorageClass::Function };

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
		CurFunc->Parameters.Add(ResultId);
		if (EnableUbsan)
		{
			PatchVarInitializedRange(&LocalVariables[ResultId]);
		}
	}

	void SpvValidator::Visit(const SpvOpLoad* Inst)
	{
		if (!LocalPointers.contains(Inst->GetPointer()))
		{
			return;
		}

		SpvPointer* Pointer = &LocalPointers[Inst->GetPointer()];
		if (!VarInitializedRange.Contains(Pointer->Var->Id))
		{
			return;
		}

		if (EnableUbsan)
		{
			SpvType* VarType = Pointer->Var->Type;
			SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
			TArray<TUniquePtr<SpvInstruction>> GetAccessInsts;
			{
				SpvId StartInitUnit = GetAccess(VarType, Pointer->Indexes, GetAccessInsts);

				SpvId EndInitUnit = Patcher.NewId();
				auto AddOp = MakeUnique<SpvOpIAdd>(IntType, StartInitUnit, Patcher.FindOrAddConstant(GetTypeByteSize(Pointer->Type->PointeeType) / 4));
				AddOp->SetId(EndInitUnit);
				GetAccessInsts.Add(MoveTemp(AddOp));

				PatchValidateInitializedRangeFunc(Pointer->Var->Id);
				SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
				auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, ValidateInitializedRangeFuncIds[Pointer->Var->Id], TArray<SpvId>{StartInitUnit, EndInitUnit});
				FuncCallOp->SetId(Patcher.NewId());
				GetAccessInsts.Add(MoveTemp(FuncCallOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(GetAccessInsts));
		}
	}

	void SpvValidator::Visit(const SpvOpStore* Inst)
	{
		if (!LocalPointers.contains(Inst->GetPointer()))
		{
			return;
		}

		SpvPointer* Pointer = &LocalPointers[Inst->GetPointer()];
		if (!VarInitializedRange.Contains(Pointer->Var->Id))
		{
			return;
		}

		if (EnableUbsan)
		{
			SpvType* VarType = Pointer->Var->Type;
			SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
			TArray<TUniquePtr<SpvInstruction>> GetAccessInsts;
			{
				SpvId StartInitUnit = GetAccess(VarType, Pointer->Indexes, GetAccessInsts);

				SpvId EndInitUnit = Patcher.NewId();
				auto AddOp = MakeUnique<SpvOpIAdd>(IntType, StartInitUnit, Patcher.FindOrAddConstant(GetTypeByteSize(Pointer->Type->PointeeType) / 4));
				AddOp->SetId(EndInitUnit);
				GetAccessInsts.Add(MoveTemp(AddOp));

				PatchUpdateInitializedRangeFunc(Pointer->Var->Id);
				SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
				auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, UpdateInitializedRangeFuncIds[Pointer->Var->Id], TArray<SpvId>{StartInitUnit, EndInitUnit});
				FuncCallOp->SetId(Patcher.NewId());
				GetAccessInsts.Add(MoveTemp(FuncCallOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value() + Inst->GetWordLen().value(), MoveTemp(GetAccessInsts));
		}
	}

	void SpvValidator::Visit(const SpvOpAccessChain* Inst)
	{
		if (!LocalPointers.contains(Inst->GetBasePointer()))
		{
			return;
		}

		SpvId ResultId = Inst->GetId().value();
		SpvPointerType* PointerType = static_cast<SpvPointerType*>(Context.Types[Inst->GetResultType()].Get());
		SpvPointer* BasePointer = &LocalPointers[Inst->GetBasePointer()];
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

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, LogicalOr);
				AnyOp->SetId(Any);
				PowValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				PowValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				PowValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				PowValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(PowValidationInsts);
				PowValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				PowValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(PowValidationInsts));
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
				SpvId IEqual = Patcher.NewId();;
				auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolResultType, Inst->GetX(), Zero);
				IEqualOp->SetId(IEqual);
				NormalizeValidationInsts.Add(MoveTemp(IEqualOp));

				SpvId All = Patcher.NewId();
				auto AllOp = MakeUnique<SpvOpAll>(BoolType, IEqual);
				AllOp->SetId(All);
				NormalizeValidationInsts.Add(MoveTemp(AllOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				NormalizeValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				NormalizeValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(All, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				NormalizeValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(NormalizeValidationInsts);
				NormalizeValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				NormalizeValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(NormalizeValidationInsts));
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

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, FOrdGreaterThan);
				AnyOp->SetId(Any);
				AsinValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				AsinValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				AsinValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				AsinValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(AsinValidationInsts);
				AsinValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				AsinValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AsinValidationInsts));
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

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, FOrdGreaterThan);
				AnyOp->SetId(Any);
				AcosValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				AcosValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				AcosValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				AcosValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(AcosValidationInsts);
				AcosValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				AcosValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AcosValidationInsts));
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

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, LogicalAnd);
				AnyOp->SetId(Any);
				Atan2ValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				Atan2ValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				Atan2ValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				Atan2ValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(Atan2ValidationInsts);
				Atan2ValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				Atan2ValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(Atan2ValidationInsts));
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

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, FOrdGreaterThan);
				AnyOp->SetId(Any);
				ClampValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ClampValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				ClampValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ClampValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ClampValidationInsts);
				ClampValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ClampValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(ClampValidationInsts));
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

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, UGreaterThan);
				AnyOp->SetId(Any);
				ClampValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ClampValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				ClampValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ClampValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ClampValidationInsts);
				ClampValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ClampValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(ClampValidationInsts));
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

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, SGreaterThan);
				AnyOp->SetId(Any);
				ClampValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ClampValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				ClampValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ClampValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ClampValidationInsts);
				ClampValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ClampValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(ClampValidationInsts));
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

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, FOrdGreaterThanEqual);
				AnyOp->SetId(Any);
				SmoothStepValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				SmoothStepValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				SmoothStepValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				SmoothStepValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(SmoothStepValidationInsts);
				SmoothStepValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				SmoothStepValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(SmoothStepValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvOpConvertFToU* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			SpvId UInt64Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(64, 0));
			SpvId UInt64ResultType = GetScalarOrVectorTypeId(ResultType, UInt64Type);
			TArray<TUniquePtr<SpvInstruction>> ConvertFValidationInsts;
			{
				SpvId Zero = GetScalarOrVectorId(ResultType, 0.0f);
				SpvId FOrdLessThan = Patcher.NewId();
				auto FOrdLessThanOp = MakeUnique<SpvOpFOrdLessThan>(BoolResultType, Inst->GetFloatValue(), Zero);
				FOrdLessThanOp->SetId(FOrdLessThan);
				ConvertFValidationInsts.Add(MoveTemp(FOrdLessThanOp));

				SpvId Maximum = GetScalarOrVectorId(ResultType, (uint64)std::numeric_limits<uint32>::max());

				SpvId UInt64Value = Patcher.NewId();
				auto UInt64ValueOp = MakeUnique<SpvOpConvertFToU>(UInt64ResultType, Inst->GetFloatValue());
				UInt64ValueOp->SetId(UInt64Value);
				ConvertFValidationInsts.Add(MoveTemp(UInt64ValueOp));

				SpvId UGreaterThan = Patcher.NewId();;
				auto UGreaterThanOp = MakeUnique<SpvOpUGreaterThan>(BoolResultType, UInt64Value, Maximum);
				UGreaterThanOp->SetId(UGreaterThan);
				ConvertFValidationInsts.Add(MoveTemp(UGreaterThanOp));

				SpvId LogicalOr = Patcher.NewId();
				auto LogicalOrOp = MakeUnique<SpvOpLogicalOr>(BoolResultType, FOrdLessThan, UGreaterThan);
				LogicalOrOp->SetId(LogicalOr);
				ConvertFValidationInsts.Add(MoveTemp(LogicalOrOp));

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, LogicalOr);
				AnyOp->SetId(Any);
				ConvertFValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ConvertFValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				ConvertFValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ConvertFValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ConvertFValidationInsts);
				ConvertFValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ConvertFValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(ConvertFValidationInsts));
		}
	}

	void SpvValidator::Visit(const SpvOpConvertFToS* Inst)
	{
		if (EnableUbsan)
		{
			SpvType* ResultType = Context.Types[Inst->GetResultType()].Get();
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId BoolResultType = GetScalarOrVectorTypeId(ResultType, BoolType);
			SpvId Int64Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(64, 1));
			SpvId Int64ResultType = GetScalarOrVectorTypeId(ResultType, Int64Type);
			TArray<TUniquePtr<SpvInstruction>> ConvertFValidationInsts;
			{
				SpvId Minimum = GetScalarOrVectorId(ResultType, (int64)std::numeric_limits<int32>::max());
				SpvId Maximum = GetScalarOrVectorId(ResultType, (int64)std::numeric_limits<int32>::max());

				SpvId Int64Value = Patcher.NewId();
				auto Int64ValueOp = MakeUnique<SpvOpConvertFToU>(Int64ResultType, Inst->GetFloatValue());
				Int64ValueOp->SetId(Int64Value);
				ConvertFValidationInsts.Add(MoveTemp(Int64ValueOp));

				SpvId SLessThan = Patcher.NewId();
				auto SLessThanOp = MakeUnique<SpvOpSLessThan>(BoolResultType, Int64Value, Minimum);
				SLessThanOp->SetId(SLessThan);
				ConvertFValidationInsts.Add(MoveTemp(SLessThanOp));

				SpvId SGreaterThan = Patcher.NewId();;
				auto SGreaterThanOp = MakeUnique<SpvOpSGreaterThan>(BoolResultType, Int64Value, Maximum);
				SGreaterThanOp->SetId(SGreaterThan);
				ConvertFValidationInsts.Add(MoveTemp(SGreaterThanOp));

				SpvId LogicalOr = Patcher.NewId();
				auto LogicalOrOp = MakeUnique<SpvOpLogicalOr>(BoolResultType, SLessThan, SGreaterThan);
				LogicalOrOp->SetId(LogicalOr);
				ConvertFValidationInsts.Add(MoveTemp(LogicalOrOp));

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, LogicalOr);
				AnyOp->SetId(Any);
				ConvertFValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ConvertFValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				ConvertFValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ConvertFValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ConvertFValidationInsts);
				ConvertFValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ConvertFValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(ConvertFValidationInsts));
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

				SpvId Any = Patcher.NewId();
				auto AnyOp = MakeUnique<SpvOpAny>(BoolType, LogicalAnd);
				AnyOp->SetId(Any);
				SRemValidationInsts.Add(MoveTemp(AnyOp));

				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				SRemValidationInsts.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
				SRemValidationInsts.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				SRemValidationInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(SRemValidationInsts);
				SRemValidationInsts.Add(MakeUnique<SpvOpBranch>(FalseLabel));

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				SRemValidationInsts.Add(MoveTemp(FalseLabelOp));
			}
			Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(SRemValidationInsts));
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
			SpvId ArrType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeArray>(IntType, Patcher.FindOrAddConstant(MaxIndexNum)));

			TArray<SpvId> ParamTypes;
			if (MaxIndexNum > 0)
			{
				ParamTypes.Add(ArrType);
			}
			
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(IntType, ParamTypes));
			auto FuncOp = MakeUnique<SpvOpFunction>(IntType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(GetAccessFuncId);
			GetAccessFuncInsts.Add(MoveTemp(FuncOp));

			SpvId IndexParam;
			if (MaxIndexNum > 0)
			{
				IndexParam = Patcher.NewId();
				auto IndexParamOp = MakeUnique<SpvOpFunctionParameter>(ArrType);
				IndexParamOp->SetId(IndexParam);
				GetAccessFuncInsts.Add(MoveTemp(IndexParamOp));
				Patcher.AddDebugName(MakeUnique<SpvOpName>(IndexParam, "_Indexes_"));
			}

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			GetAccessFuncInsts.Add(MoveTemp(LabelOp));

			if (MaxIndexNum > 0)
			{
				SpvId FirstIndex = Patcher.NewId();
				auto ExtractOp = MakeUnique<SpvOpCompositeExtract>(IntType, IndexParam, TArray<uint32>{0});
				ExtractOp->SetId(FirstIndex);
				GetAccessFuncInsts.Add(MoveTemp(ExtractOp));

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
							SpvId NewIndex = Patcher.NewId();
							auto ExtractOp = MakeUnique<SpvOpCompositeExtract>(IntType, IndexParam, TArray<uint32>{(uint32)i + 1});
							ExtractOp->SetId(NewIndex);
							GetAccessFuncInsts.Add(MoveTemp(ExtractOp));
							MemberIndexes.Add(NewIndex);
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
							SpvId NewIndex = Patcher.NewId();
							auto ExtractOp = MakeUnique<SpvOpCompositeExtract>(IntType, IndexParam, TArray<uint32>{(uint32)i + 1});
							ExtractOp->SetId(NewIndex);
							GetAccessFuncInsts.Add(MoveTemp(ExtractOp));
							MemberIndexes.Add(NewIndex);
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
							SpvId NewIndex = Patcher.NewId();
							auto ExtractOp = MakeUnique<SpvOpCompositeExtract>(IntType, IndexParam, TArray<uint32>{(uint32)i + 1});
							ExtractOp->SetId(NewIndex);
							GetAccessFuncInsts.Add(MoveTemp(ExtractOp));
							MemberIndexes.Add(NewIndex);
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
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
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
				SpvId UIntPointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UIntType));
				SpvId DebuggerBufferStorage = Patcher.NewId();
				SpvId Zero = Patcher.FindOrAddConstant(0u);
				auto DebuggerBufferStorageOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, Zero});
				DebuggerBufferStorageOp->SetId(DebuggerBufferStorage);
				AppendErrorFuncInsts.Add(MoveTemp(DebuggerBufferStorageOp));

				SpvId OriginalValue = Patcher.NewId();
				auto AtomicOp = MakeUnique<SpvOpAtomicIAdd>(UIntType, DebuggerBufferStorage, Patcher.FindOrAddConstant((uint32)SpvMemoryScope::Device),
					Patcher.FindOrAddConstant((uint32)SpvMemorySemantics::None), Patcher.FindOrAddConstant(8u));
				AtomicOp->SetId(OriginalValue);
				AppendErrorFuncInsts.Add(MoveTemp(AtomicOp));

				SpvId OriginalValuePlus4 = Patcher.NewId();
				auto Add4Op = MakeUnique<SpvOpIAdd>(UIntType, OriginalValue, Patcher.FindOrAddConstant(4u));
				Add4Op->SetId(OriginalValuePlus4);
				AppendErrorFuncInsts.Add(MoveTemp(Add4Op));

				SpvId OriginalValuePlus12 = Patcher.NewId();
				auto Add12Op = MakeUnique<SpvOpIAdd>(UIntType, OriginalValue, Patcher.FindOrAddConstant(12u));
				Add12Op->SetId(OriginalValuePlus12);
				AppendErrorFuncInsts.Add(MoveTemp(Add12Op));

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
					SpvId AlignedOffset = Patcher.NewId();
					auto AlignedOffsetOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, OriginalValuePlus4, Patcher.FindOrAddConstant(2u));
					AlignedOffsetOp->SetId(AlignedOffset);
					AppendErrorFuncInsts.Add(MoveTemp(AlignedOffsetOp));

					SpvId DebuggerBufferStorage = Patcher.NewId();
					auto DebuggerBufferStorageOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, AlignedOffset});
					DebuggerBufferStorageOp->SetId(DebuggerBufferStorage);
					AppendErrorFuncInsts.Add(MoveTemp(DebuggerBufferStorageOp));

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

					AppendErrorFuncInsts.Add(MakeUnique<SpvOpStore>(DebuggerBufferStorage, UIntFragCoordXY));
				}
			}

			AppendErrorFuncInsts.Add(MakeUnique<SpvOpReturn>());
			AppendErrorFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(AppendErrorFuncInsts));
	}

	void SpvValidator::PatchValidateInitializedRangeFunc(SpvId VarId)
	{
		if (ValidateInitializedRangeFuncIds.Contains(VarId))
		{
			return;
		}

		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId IntPointerFunctionType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Function, IntType));
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());

		SpvId ValidateInitializedRangeFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(ValidateInitializedRangeFuncId, FString::Printf(TEXT("_ValidateInitializedRange_%d_"), VarId.GetValue())));
		TArray<TUniquePtr<SpvInstruction>> ValidateRangeFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, TArray<SpvId>{IntPointerFunctionType, IntPointerFunctionType}));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(ValidateInitializedRangeFuncId);
			ValidateRangeFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StartParam = Patcher.NewId();
			auto StartParamOp = MakeUnique<SpvOpFunctionParameter>(IntPointerFunctionType);
			StartParamOp->SetId(StartParam);
			ValidateRangeFuncInsts.Add(MoveTemp(StartParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StartParam, "_Start_"));

			SpvId EndParam = Patcher.NewId();
			auto EndParamOp = MakeUnique<SpvOpFunctionParameter>(IntPointerFunctionType);
			EndParamOp->SetId(EndParam);
			ValidateRangeFuncInsts.Add(MoveTemp(EndParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(EndParam, "_End_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			ValidateRangeFuncInsts.Add(MoveTemp(LabelOp));

			SpvId ForCheckLabel = Patcher.NewId();
			SpvId ForBodyLabel = Patcher.NewId();
			SpvId ForMergeLabel = Patcher.NewId();
			SpvId ForContinueLabel = Patcher.NewId();
			ValidateRangeFuncInsts.Add(MakeUnique<SpvOpBranch>(ForCheckLabel));
			{
				auto LabelOp = MakeUnique<SpvOpLabel>();
				LabelOp->SetId(ForCheckLabel);
				ValidateRangeFuncInsts.Add(MoveTemp(LabelOp));

				SpvId LoadedStart = Patcher.NewId();
				auto LoadedStartOp = MakeUnique<SpvOpLoad>(IntType, StartParam);
				LoadedStartOp->SetId(LoadedStart);
				ValidateRangeFuncInsts.Add(MoveTemp(LoadedStartOp));

				SpvId LoadedEnd = Patcher.NewId();
				auto LoadedEndOp = MakeUnique<SpvOpLoad>(IntType, EndParam);
				LoadedEndOp->SetId(LoadedEnd);
				ValidateRangeFuncInsts.Add(MoveTemp(LoadedEndOp));

				SpvId SLessThan = Patcher.NewId();
				auto SLessThanOp = MakeUnique<SpvOpSLessThan>(BoolType, LoadedStart, LoadedEnd);
				SLessThanOp->SetId(SLessThan);
				ValidateRangeFuncInsts.Add(MoveTemp(SLessThanOp));

				ValidateRangeFuncInsts.Add(MakeUnique<SpvOpLoopMerge>(ForMergeLabel, ForContinueLabel, SpvLoopControl::None));
				ValidateRangeFuncInsts.Add(MakeUnique<SpvOpBranchConditional>(SLessThan, ForBodyLabel, ForMergeLabel));
			}

			{
				auto LabelOp = MakeUnique<SpvOpLabel>();
				LabelOp->SetId(ForBodyLabel);
				ValidateRangeFuncInsts.Add(MoveTemp(LabelOp));

				SpvId LoadedStart = Patcher.NewId();
				auto LoadedStartOp = MakeUnique<SpvOpLoad>(IntType, StartParam);
				LoadedStartOp->SetId(LoadedStart);
				ValidateRangeFuncInsts.Add(MoveTemp(LoadedStartOp));

				SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
				SpvId InitializedRangeStorage = Patcher.NewId();
				auto InitializedRangeStorageOp = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[VarId], TArray<SpvId>{LoadedStart});
				InitializedRangeStorageOp->SetId(InitializedRangeStorage);
				ValidateRangeFuncInsts.Add(MoveTemp(InitializedRangeStorageOp));

				SpvId LoadedInitializedRange = Patcher.NewId();
				auto LoadedInitializedRangeOp = MakeUnique<SpvOpLoad>(IntType, InitializedRangeStorage);
				LoadedInitializedRangeOp->SetId(LoadedInitializedRange);
				ValidateRangeFuncInsts.Add(MoveTemp(LoadedInitializedRangeOp));

				SpvId IEqual = Patcher.NewId();;
				auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolType, LoadedInitializedRange, Patcher.FindOrAddConstant(0));
				IEqualOp->SetId(IEqual);
				ValidateRangeFuncInsts.Add(MoveTemp(IEqualOp));
				
				auto TrueLabel = Patcher.NewId();
				auto FalseLabel = Patcher.NewId();
				ValidateRangeFuncInsts.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
				ValidateRangeFuncInsts.Add(MakeUnique<SpvOpBranchConditional>(IEqual, TrueLabel, FalseLabel));

				auto TrueLabelOp = MakeUnique<SpvOpLabel>();
				TrueLabelOp->SetId(TrueLabel);
				ValidateRangeFuncInsts.Add(MoveTemp(TrueLabelOp));
				AppendError(ValidateRangeFuncInsts);
				ValidateRangeFuncInsts.Add(MakeUnique<SpvOpReturn>());

				auto FalseLabelOp = MakeUnique<SpvOpLabel>();
				FalseLabelOp->SetId(FalseLabel);
				ValidateRangeFuncInsts.Add(MoveTemp(FalseLabelOp));		
				ValidateRangeFuncInsts.Add(MakeUnique<SpvOpBranch>(ForContinueLabel));
			}

			{
				auto LabelOp = MakeUnique<SpvOpLabel>();
				LabelOp->SetId(ForContinueLabel);
				ValidateRangeFuncInsts.Add(MoveTemp(LabelOp));

				SpvId LoadedStart = Patcher.NewId();
				auto LoadedStartOp = MakeUnique<SpvOpLoad>(IntType, StartParam);
				LoadedStartOp->SetId(LoadedStart);
				ValidateRangeFuncInsts.Add(MoveTemp(LoadedStartOp));

				SpvId AddRet = Patcher.NewId();
				auto AddOp = MakeUnique<SpvOpIAdd>(IntType, LoadedStart, Patcher.FindOrAddConstant(1));
				AddOp->SetId(AddRet);
				ValidateRangeFuncInsts.Add(MoveTemp(AddOp));

				ValidateRangeFuncInsts.Add(MakeUnique<SpvOpStore>(StartParam, AddRet));
				ValidateRangeFuncInsts.Add(MakeUnique<SpvOpBranch>(ForCheckLabel));
			}

			{
				auto LabelOp = MakeUnique<SpvOpLabel>();
				LabelOp->SetId(ForMergeLabel);
				ValidateRangeFuncInsts.Add(MoveTemp(LabelOp));
			}

			ValidateRangeFuncInsts.Add(MakeUnique<SpvOpReturn>());
			ValidateRangeFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(ValidateRangeFuncInsts));
		ValidateInitializedRangeFuncIds.Add(VarId, ValidateInitializedRangeFuncId);
	}

	void SpvValidator::PatchUpdateInitializedRangeFunc(SpvId VarId)
	{
		if (UpdateInitializedRangeFuncIds.Contains(VarId))
		{
			return;
		}

		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId IntPointerFunctionType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Function, IntType));
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());

		SpvId UpdateInitializedRangeFuncId = Patcher.NewId();
		Patcher.AddDebugName(MakeUnique<SpvOpName>(UpdateInitializedRangeFuncId, FString::Printf(TEXT("_UpdateInitializedRange_%d_"), VarId.GetValue())));
		TArray<TUniquePtr<SpvInstruction>> UpdateFuncInsts;
		{
			SpvId FuncType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFunction>(VoidType, TArray<SpvId>{IntPointerFunctionType, IntPointerFunctionType}));
			auto FuncOp = MakeUnique<SpvOpFunction>(VoidType, SpvFunctionControl::None, FuncType);
			FuncOp->SetId(UpdateInitializedRangeFuncId);
			UpdateFuncInsts.Add(MoveTemp(FuncOp));

			SpvId StartParam = Patcher.NewId();
			auto StartParamOp = MakeUnique<SpvOpFunctionParameter>(IntPointerFunctionType);
			StartParamOp->SetId(StartParam);
			UpdateFuncInsts.Add(MoveTemp(StartParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(StartParam, "_Start_"));

			SpvId EndParam = Patcher.NewId();
			auto EndParamOp = MakeUnique<SpvOpFunctionParameter>(IntPointerFunctionType);
			EndParamOp->SetId(EndParam);
			UpdateFuncInsts.Add(MoveTemp(EndParamOp));
			Patcher.AddDebugName(MakeUnique<SpvOpName>(EndParam, "_End_"));

			auto LabelOp = MakeUnique<SpvOpLabel>();
			LabelOp->SetId(Patcher.NewId());
			UpdateFuncInsts.Add(MoveTemp(LabelOp));

			SpvId ForCheckLabel = Patcher.NewId();
			SpvId ForBodyLabel = Patcher.NewId();
			SpvId ForMergeLabel = Patcher.NewId();
			SpvId ForContinueLabel = Patcher.NewId();
			UpdateFuncInsts.Add(MakeUnique<SpvOpBranch>(ForCheckLabel));
			{
				auto LabelOp = MakeUnique<SpvOpLabel>();
				LabelOp->SetId(ForCheckLabel);
				UpdateFuncInsts.Add(MoveTemp(LabelOp));

				SpvId LoadedStart = Patcher.NewId();
				auto LoadedStartOp = MakeUnique<SpvOpLoad>(IntType, StartParam);
				LoadedStartOp->SetId(LoadedStart);
				UpdateFuncInsts.Add(MoveTemp(LoadedStartOp));

				SpvId LoadedEnd = Patcher.NewId();
				auto LoadedEndOp = MakeUnique<SpvOpLoad>(IntType, EndParam);
				LoadedEndOp->SetId(LoadedEnd);
				UpdateFuncInsts.Add(MoveTemp(LoadedEndOp));

				SpvId SLessThan = Patcher.NewId();
				auto SLessThanOp = MakeUnique<SpvOpSLessThan>(BoolType, LoadedStart, LoadedEnd);
				SLessThanOp->SetId(SLessThan);
				UpdateFuncInsts.Add(MoveTemp(SLessThanOp));

				UpdateFuncInsts.Add(MakeUnique<SpvOpLoopMerge>(ForMergeLabel, ForContinueLabel, SpvLoopControl::None));
				UpdateFuncInsts.Add(MakeUnique<SpvOpBranchConditional>(SLessThan, ForBodyLabel, ForMergeLabel));
			}

			{
				auto LabelOp = MakeUnique<SpvOpLabel>();
				LabelOp->SetId(ForBodyLabel);
				UpdateFuncInsts.Add(MoveTemp(LabelOp));

				SpvId LoadedStart = Patcher.NewId();
				auto LoadedStartOp = MakeUnique<SpvOpLoad>(IntType, StartParam);
				LoadedStartOp->SetId(LoadedStart);
				UpdateFuncInsts.Add(MoveTemp(LoadedStartOp));

				SpvId UIntPointerPrivateType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Private, UIntType));
				SpvId InitializedRangeStorage = Patcher.NewId();
				auto InitializedRangeStorageOp = MakeUnique<SpvOpAccessChain>(UIntPointerPrivateType, VarInitializedRange[VarId], TArray<SpvId>{LoadedStart});
				InitializedRangeStorageOp->SetId(InitializedRangeStorage);
				UpdateFuncInsts.Add(MoveTemp(InitializedRangeStorageOp));
				UpdateFuncInsts.Add(MakeUnique<SpvOpStore>(InitializedRangeStorage, Patcher.FindOrAddConstant(1u)));
				UpdateFuncInsts.Add(MakeUnique<SpvOpBranch>(ForContinueLabel));
			}
			
			{
				auto LabelOp = MakeUnique<SpvOpLabel>();
				LabelOp->SetId(ForContinueLabel);
				UpdateFuncInsts.Add(MoveTemp(LabelOp));

				SpvId LoadedStart = Patcher.NewId();
				auto LoadedStartOp = MakeUnique<SpvOpLoad>(IntType, StartParam);
				LoadedStartOp->SetId(LoadedStart);
				UpdateFuncInsts.Add(MoveTemp(LoadedStartOp));

				SpvId AddRet = Patcher.NewId();
				auto AddOp = MakeUnique<SpvOpIAdd>(IntType, LoadedStart, Patcher.FindOrAddConstant(1));
				AddOp->SetId(AddRet);
				UpdateFuncInsts.Add(MoveTemp(AddOp));

				UpdateFuncInsts.Add(MakeUnique<SpvOpStore>(StartParam, AddRet));
				UpdateFuncInsts.Add(MakeUnique<SpvOpBranch>(ForCheckLabel));
			}

			{
				auto LabelOp = MakeUnique<SpvOpLabel>();
				LabelOp->SetId(ForMergeLabel);
				UpdateFuncInsts.Add(MoveTemp(LabelOp));
			}

			UpdateFuncInsts.Add(MakeUnique<SpvOpReturn>());
			UpdateFuncInsts.Add(MakeUnique<SpvOpFunctionEnd>());
		}
		Patcher.AddFunction(MoveTemp(UpdateFuncInsts));
		UpdateInitializedRangeFuncIds.Add(VarId, UpdateInitializedRangeFuncId);
	}

	void SpvValidator::AppendError(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendErrorFuncId);
		FuncCallOp->SetId(Patcher.NewId());
		InstList.Add(MoveTemp(FuncCallOp));
	}

	SpvId SpvValidator::GetAccess(SpvType* VarType, const TArray<SpvId>& Indexes, TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		int32 MaxIndexNum = GetMaxIndexNum(VarType);
		PatchGetAccessFunc(VarType);

		SpvId GetAccessFuncId = GetAccessFuncIds[VarType];
		SpvId IntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 1));
		SpvId ArrType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeArray>(IntType, Patcher.FindOrAddConstant(MaxIndexNum)));

		TArray<SpvId> Arguments;
		if (MaxIndexNum > 0)
		{
			SpvId IndexArr = Patcher.NewId();
			TArray<SpvId> Constituents;
			for (SpvId Index : Indexes)
			{
				SpvId BitCastValue = Patcher.NewId();
				auto BitCastOp = MakeUnique<SpvOpBitcast>(IntType, Index);
				BitCastOp->SetId(BitCastValue);
				InstList.Add(MoveTemp(BitCastOp));
				Constituents.Add(BitCastValue);
			}
			while (MaxIndexNum - Constituents.Num() > 0)
			{
				Constituents.Add(Patcher.FindOrAddConstant(0));
			}

			auto IndexArrOp = MakeUnique<SpvOpCompositeConstruct>(ArrType, Constituents);
			IndexArrOp->SetId(IndexArr);
			InstList.Add(MoveTemp(IndexArrOp));
			Arguments.Add(IndexArr);
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

			SpvId Any = Patcher.NewId();
			auto AnyOp = MakeUnique<SpvOpAny>(BoolType, ComparisonId);
			AnyOp->SetId(Any);
			InstList.Add(MoveTemp(AnyOp));

			auto TrueLabel = Patcher.NewId();
			auto FalseLabel = Patcher.NewId();
			InstList.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
			InstList.Add(MakeUnique<SpvOpBranchConditional>(Any, TrueLabel, FalseLabel));

			auto TrueLabelOp = MakeUnique<SpvOpLabel>();
			TrueLabelOp->SetId(TrueLabel);
			InstList.Add(MoveTemp(TrueLabelOp));
			AppendError(InstList);
			InstList.Add(MakeUnique<SpvOpBranch>(FalseLabel));

			auto FalseLabelOp = MakeUnique<SpvOpLabel>();
			FalseLabelOp->SetId(FalseLabel);
			InstList.Add(MoveTemp(FalseLabelOp));
		}
		Patcher.AddInstructions(OffsetEval(), MoveTemp(InstList));
	}
}
