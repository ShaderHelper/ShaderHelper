#include "CommonHeader.h"
#include "SpirvPixelDebugger.h"

namespace FW
{
	SpvPixelDebuggerVisitor::SpvPixelDebuggerVisitor(SpvPixelDebuggerContext& InPixelContext, GpuShaderLanguage InLanguage, bool InEnableUbsan)
	: SpvDebuggerVisitor(InPixelContext, InLanguage, InEnableUbsan)
	{}

	void SpvPixelDebuggerVisitor::ParseInternal()
	{
		PatchFragCoordBuiltIn(Patcher, Context);

		// Patch DebuggerParams uniform buffer before base class processing
		DebuggerParams = PatchDebuggerParams(Patcher, BindingShaderStage::Pixel);
		
		// Call base class implementation
		SpvDebuggerVisitor::ParseInternal();
	}

	bool SpvPixelDebuggerVisitor::PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList)
	{
		SpvId BoolType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeBool>());
		SpvId Bool2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(BoolType, 2));
		SpvId UIntType = Patcher.FindOrAddType(MakeUnique<SpvOpTypeInt>(32, 0));
		SpvId UInt2Type = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(UIntType, 2));

		SpvId UIntFragCoordXY = PatchLoadFragCoordXY(Patcher, Context, InstList);

		// Load PixelCoord from DebuggerParams uniform buffer instead of using constant
		SpvId UInt2PointerUniformType = Patcher.FindOrAddType(MakeUnique<SpvOpTypePointer>(SpvStorageClass::Uniform, UInt2Type));
		SpvId PixelCoordPtr = Patcher.NewId();
		auto PixelCoordPtrOp = MakeUnique<SpvOpAccessChain>(UInt2PointerUniformType, DebuggerParams, TArray<SpvId>{Patcher.FindOrAddConstant(0u)});
		PixelCoordPtrOp->SetId(PixelCoordPtr);
		InstList.Add(MoveTemp(PixelCoordPtrOp));

		SpvId PixelCoord = Patcher.NewId();
		auto PixelCoordOp = MakeUnique<SpvOpLoad>(UInt2Type, PixelCoordPtr);
		PixelCoordOp->SetId(PixelCoord);
		InstList.Add(MoveTemp(PixelCoordOp));

		SpvId IEqual = Patcher.NewId();;
		auto IEqualOp = MakeUnique<SpvOpIEqual>(Bool2Type, UIntFragCoordXY, PixelCoord);
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

		return true;
	}

	void SpvPixelDebuggerVisitor::Visit(const SpvOpKill* Inst)
	{
		AppendTag([&] {return Inst->GetWordOffset().value(); }, SpvDebuggerStateType::Kill);
	}

}
