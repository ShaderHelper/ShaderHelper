#include "CommonHeader.h"
#include "SpirvPixelDebugger.h"

namespace FW
{
	SpvPixelDebuggerVisitor::SpvPixelDebuggerVisitor(SpvPixelDebuggerContext& InPixelContext, bool InEnableUbsan, bool InGlobalValidation)
	: SpvDebuggerVisitor(InPixelContext, InEnableUbsan, InGlobalValidation)
	{}

	void SpvPixelDebuggerVisitor::PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
		SpvId Float2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 2));
		SpvId Float4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 4));
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UInt2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 2));
		if (GlobalValidation)
		{
			SpvId UIntPointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UIntType));
			SpvId DebuggerBufferStorage = Patcher.NewId();
			SpvId Zero = Patcher.FindOrAddConstant(0u);
			auto DebuggerBufferStorageOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, Zero});
			DebuggerBufferStorageOp->SetId(DebuggerBufferStorage);
			InstList.Add(MoveTemp(DebuggerBufferStorageOp));

			SpvId OriginalValue = Patcher.NewId();
			auto AtomicOp = MakeUnique<SpvOpAtomicIAdd>(UIntType, DebuggerBufferStorage, Patcher.FindOrAddConstant((uint32)SpvMemoryScope::Device),
				Patcher.FindOrAddConstant((uint32)SpvMemorySemantics::None), Patcher.FindOrAddConstant(8u));
			AtomicOp->SetId(OriginalValue);
			InstList.Add(MoveTemp(AtomicOp));

			SpvId OriginalValuePlus4 = Patcher.NewId();
			auto Add4Op = MakeUnique<SpvOpIAdd>(UIntType, OriginalValue, Patcher.FindOrAddConstant(4u));
			Add4Op->SetId(OriginalValuePlus4);
			InstList.Add(MoveTemp(Add4Op));

			SpvId OriginalValuePlus12 = Patcher.NewId();
			auto Add12Op = MakeUnique<SpvOpIAdd>(UIntType, OriginalValue, Patcher.FindOrAddConstant(12u));
			Add12Op->SetId(OriginalValuePlus12);
			InstList.Add(MoveTemp(Add12Op));

			SpvId UGreaterThan = Patcher.NewId();;
			auto UGreaterThanOp = MakeUnique<SpvOpUGreaterThan>(BoolType, OriginalValuePlus12, Patcher.FindOrAddConstant(1024u));
			UGreaterThanOp->SetId(UGreaterThan);
			InstList.Add(MoveTemp(UGreaterThanOp));

			auto TrueLabel = Patcher.NewId();
			auto FalseLabel = Patcher.NewId();
			InstList.Add(MakeUnique<SpvOpSelectionMerge>(FalseLabel, SpvSelectionControl::None));
			InstList.Add(MakeUnique<SpvOpBranchConditional>(UGreaterThan, TrueLabel, FalseLabel));

			auto TrueLabelOp = MakeUnique<SpvOpLabel>();
			TrueLabelOp->SetId(TrueLabel);
			InstList.Add(MoveTemp(TrueLabelOp));
			InstList.Add(MakeUnique<SpvOpReturn>());

			auto FalseLabelOp = MakeUnique<SpvOpLabel>();
			FalseLabelOp->SetId(FalseLabel);
			InstList.Add(MoveTemp(FalseLabelOp));

			{
				SpvId AlignedOffset = Patcher.NewId();
				auto AlignedOffsetOp = MakeUnique<SpvOpShiftRightLogical>(UIntType, OriginalValuePlus4, Patcher.FindOrAddConstant(2u));
				AlignedOffsetOp->SetId(AlignedOffset);
				InstList.Add(MoveTemp(AlignedOffsetOp));

				SpvId DebuggerBufferStorage = Patcher.NewId();
				auto DebuggerBufferStorageOp = MakeUnique<SpvOpAccessChain>(UIntPointerUniformType, DebuggerBuffer, TArray<SpvId>{Zero, AlignedOffset});
				DebuggerBufferStorageOp->SetId(DebuggerBufferStorage);
				InstList.Add(MoveTemp(DebuggerBufferStorageOp));

				SpvId LoadedFragCoord = Patcher.NewId();
				auto LoadedFragCoordOp = MakeUnique<SpvOpLoad>(Float4Type, Context.BuiltIns[SpvBuiltIn::FragCoord]);
				LoadedFragCoordOp->SetId(LoadedFragCoord);
				InstList.Add(MoveTemp(LoadedFragCoordOp));

				SpvId FragCoordXY = Patcher.NewId();
				auto FragCoordXYOp = MakeUnique<SpvOpVectorShuffle>(Float2Type, LoadedFragCoord, LoadedFragCoord, TArray<uint32>{0, 1});
				FragCoordXYOp->SetId(FragCoordXY);
				InstList.Add(MoveTemp(FragCoordXYOp));

				SpvId UIntFragCoordXY = Patcher.NewId();
				auto UIntFragCoordXYOp = MakeUnique<SpvOpConvertFToU>(UInt2Type, FragCoordXY);
				UIntFragCoordXYOp->SetId(UIntFragCoordXY);
				InstList.Add(MoveTemp(UIntFragCoordXYOp));

				InstList.Add(MakeUnique<SpvOpStore>(DebuggerBufferStorage, UIntFragCoordXY));

				InstList.Add(MakeUnique<SpvOpReturn>());
				auto LabelOp = MakeUnique<SpvOpLabel>();
				LabelOp->SetId(Patcher.NewId());
				InstList.Add(MoveTemp(LabelOp));
			}
		}
		else
		{
			SpvPixelDebuggerContext& PixelContext = static_cast<SpvPixelDebuggerContext&>(Context);
			SpvId Bool2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(BoolType, 2));

			SpvId LoadedFragCoord = Patcher.NewId();
			auto LoadedFragCoordOp = MakeUnique<SpvOpLoad>(Float4Type, Context.BuiltIns[SpvBuiltIn::FragCoord]);
			LoadedFragCoordOp->SetId(LoadedFragCoord);
			InstList.Add(MoveTemp(LoadedFragCoordOp));

			SpvId FragCoordXY = Patcher.NewId();
			auto FragCoordXYOp = MakeUnique<SpvOpVectorShuffle>(Float2Type, LoadedFragCoord, LoadedFragCoord, TArray<uint32>{0, 1});
			FragCoordXYOp->SetId(FragCoordXY);
			InstList.Add(MoveTemp(FragCoordXYOp));

			SpvId UIntFragCoordXY = Patcher.NewId();
			auto UIntFragCoordXYOp = MakeUnique<SpvOpConvertFToU>(UInt2Type, FragCoordXY);
			UIntFragCoordXYOp->SetId(UIntFragCoordXY);
			InstList.Add(MoveTemp(UIntFragCoordXYOp));

			SpvId IEqual = Patcher.NewId();;
			auto IEqualOp = MakeUnique<SpvOpIEqual>(Bool2Type, UIntFragCoordXY, Patcher.FindOrAddConstant(PixelContext.PixelCoord));
			IEqualOp->SetId(IEqual);
			InstList.Add(MoveTemp(IEqualOp));

			SpvId All = Patcher.NewId();
			auto AllOp = MakeUnique<SpvOpAll>(BoolType, IEqual);
			AllOp->SetId(All);
			InstList.Add(MoveTemp(AllOp));

			auto TrueLabel = Patcher.NewId();
			auto FalseLabel = Patcher.NewId();
			InstList.Add(MakeUnique<SpvOpSelectionMerge>(TrueLabel, SpvSelectionControl::None));
			InstList.Add(MakeUnique<SpvOpBranchConditional>(All, TrueLabel, FalseLabel));
			auto FalseLabelOp = MakeUnique<SpvOpLabel>();
			FalseLabelOp->SetId(FalseLabel);
			InstList.Add(MoveTemp(FalseLabelOp));
			InstList.Add(MakeUnique<SpvOpReturn>());
			auto TrueLabelOp = MakeUnique<SpvOpLabel>();
			TrueLabelOp->SetId(TrueLabel);
			InstList.Add(MoveTemp(TrueLabelOp));
		}

	}

	void SpvPixelDebuggerVisitor::Visit(const SpvOpKill* Inst)
	{
		AppendTag([&] {return Inst->GetWordOffset().value(); }, SpvDebuggerStateType::Kill);
	}

}
