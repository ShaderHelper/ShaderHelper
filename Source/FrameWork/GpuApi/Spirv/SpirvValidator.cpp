#include "CommonHeader.h"
#include "SpirvValidator.h"

namespace FW
{
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
		
		int32 InstIndex = GetInstIndex(this->Insts, Context.EntryPoint);
		while (InstIndex < Insts.Num())
		{
			Insts[InstIndex]->Accept(this);
			InstIndex++;
		}
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
			AppendAnyEqualZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2());
		}
	}

	void SpvValidator::Visit(const SpvOpSDiv* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyEqualZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2());
		}
	}

	void SpvValidator::Visit(const SpvOpUMod* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyEqualZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2());
		}
	}

	void SpvValidator::Visit(const SpvOpSRem* Inst)
	{
		if (EnableUbsan)
		{
			AppendAnyEqualZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2());

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
			AppendAnyEqualZeroError([&] { return Inst->GetWordOffset().value(); }, Inst->GetResultType(), Inst->GetOperand2());
		}
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

	void SpvValidator::AppendError(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
		auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendErrorFuncId);
		FuncCallOp->SetId(Patcher.NewId());
		InstList.Add(MoveTemp(FuncCallOp));
	}

	void SpvValidator::AppendAnyEqualZeroError(const TFunction<int32()>& OffsetEval, SpvId ResultTypeId, SpvId ValueId)
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

			SpvId IEqual = Patcher.NewId();;
			auto IEqualOp = MakeUnique<SpvOpIEqual>(BoolResultType, ValueId, Zero);
			IEqualOp->SetId(IEqual);
			InstList.Add(MoveTemp(IEqualOp));

			SpvId Any = Patcher.NewId();
			auto AnyOp = MakeUnique<SpvOpAny>(BoolType, IEqual);
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
