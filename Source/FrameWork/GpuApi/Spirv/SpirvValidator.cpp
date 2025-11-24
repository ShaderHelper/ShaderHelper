#include "CommonHeader.h"
#include "SpirvValidator.h"

namespace FW
{
	SpvId SpvValidator::GetBoolTypeId(SpvType* Type)
	{
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		if (Type->IsScalar())
		{
			return BoolType;
		}
		else
		{
			SpvVectorType* VectorType = static_cast<SpvVectorType*>(Type);
			return Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(BoolType, VectorType->ElementCount));
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
			SpvId BoolResultType = GetBoolTypeId(ResultType);
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			TArray<TUniquePtr<SpvInstruction>> PowValidationInsts;
			{
				SpvId FOrdLessThan = Patcher.NewId();
				auto FOrdLessThanOp = MakeUnique<SpvOpFOrdLessThan>(BoolResultType, Inst->GetX(), Patcher.FindOrAddConstant(0.0f));
				FOrdLessThanOp->SetId(FOrdLessThan);
				PowValidationInsts.Add(MoveTemp(FOrdLessThanOp));

				SpvId FOrdEqual = Patcher.NewId();
				auto FOrdEqualOp = MakeUnique<SpvOpFOrdEqual>(BoolResultType, Inst->GetX(), Patcher.FindOrAddConstant(0.0f));
				FOrdEqualOp->SetId(FOrdEqual);
				PowValidationInsts.Add(MoveTemp(FOrdEqualOp));

				SpvId FOrdLessThanEqual = Patcher.NewId();
				auto FOrdLessThanEqualOp = MakeUnique<SpvOpFOrdLessThanEqual>(BoolResultType, Inst->GetY(), Patcher.FindOrAddConstant(0.0f));
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
			SpvId BoolResultType = GetBoolTypeId(ResultType);
			SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
			SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
			TArray<TUniquePtr<SpvInstruction>> NormalizeValidationInsts;
			{
				SpvId Zero = Patcher.FindOrAddConstant(0.0f);
				if (SpvVectorType* VecType = dynamic_cast<SpvVectorType*>(ResultType))
				{
					uint32 ElemCount = VecType->ElementCount;
					SpvId FloatVecType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, ElemCount));
					TArray<SpvId> Constituents;
					Constituents.Init(Zero, ElemCount);
					Zero = Patcher.NewId();
					auto ZeroVecOp = MakeUnique<SpvOpConstantComposite>(FloatVecType, Constituents);
					ZeroVecOp->SetId(Zero);
					NormalizeValidationInsts.Add(MoveTemp(ZeroVecOp));
				}
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
}
