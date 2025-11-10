#include "CommonHeader.h"
#include "SpirvPixelDebugger.h"
#include "GpuApi/GpuRhi.h"

namespace FW
{
	SpvPixelDebuggerVisitor::SpvPixelDebuggerVisitor(SpvPixelDebuggerContext& InPixelContext)
	: SpvDebuggerVisitor(InPixelContext)
	{}

	void SpvPixelDebuggerVisitor::PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvPixelDebuggerContext& PixelContext = static_cast<SpvPixelDebuggerContext&>(Context);

		SpvId FloatType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeFloat>(32));
		SpvId Float2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 2));
		SpvId Float4Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(FloatType, 4));
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UInt2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 2));
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		SpvId Bool2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(BoolType, 2));

		SpvId LoadedFragCoord = Patcher.NewId();
		auto LoadedFragCoordOp = MakeUnique<SpvOpLoad>(Float4Type,  Context.BuiltIns[SpvBuiltIn::FragCoord]);
		LoadedFragCoordOp->SetId(LoadedFragCoord);
		InstList.Add(MoveTemp(LoadedFragCoordOp));

		SpvId FragCoordXY = Patcher.NewId();
		auto FragCoordXYOp = MakeUnique<SpvOpVectorShuffle>(Float2Type, LoadedFragCoord, LoadedFragCoord, TArray<uint32>{0,1});
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

	void SpvPixelDebuggerVisitor::Visit(const SpvOpKill* Inst)
	{
		TArray<TUniquePtr<SpvInstruction>> AppendTagInsts;
		{
			SpvId StateType = Patcher.FindOrAddConstant((uint32)SpvDebuggerStateType::Kill);
			SpvId Line = Patcher.FindOrAddConstant((uint32)CurLine);
			SpvId VoidType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVoid>());
			auto FuncCallOp = MakeUnique<SpvOpFunctionCall>(VoidType, AppendTagFuncId, TArray<SpvId>{ StateType, Line});
			FuncCallOp->SetId(Patcher.NewId());
			AppendTagInsts.Add(MoveTemp(FuncCallOp));
		}
		Patcher.AddInstructions(Inst->GetWordOffset().value(), MoveTemp(AppendTagInsts));
	}

}
