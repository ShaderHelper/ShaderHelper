#pragma once
#include "SpirvDebugger.h"

namespace FW
{
	inline constexpr int PreviewerParamsBindingSlot = 0723;

	struct SpvPixelPreviewerContext : SpvDebuggerContext
	{
		SpvPixelPreviewerContext(const TArray<SpvBinding>& InBindings)
			: SpvDebuggerContext{ .Bindings = InBindings }
		{}
	};

	//Patches shader to output a preview variable value as fragColor.
	//A float4 global _PreviewOutput_ is assigned when PackedHeader, VarId, and CallStackHash match
	//the uniform target, and the per-match iteration counter equals TargetIteration.
	//CallStackHash tracks the call path via a reversible hash (hash = hash*31 + CallId on enter,
	//undo on return), ensuring only pixels with the same call stack contribute.
	//The original output variable is overridden.
	class FRAMEWORK_API SpvPixelPreviewerVisitor : public SpvDebuggerVisitor
	{
	public:
		SpvPixelPreviewerVisitor(SpvPixelPreviewerContext& InContext, GpuShaderLanguage InLanguage)
			: SpvDebuggerVisitor(InContext, InLanguage, /*EnableUbsan=*/false)
		{}

	protected:
		void ParseInternal() override;
		bool PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList) override;
		void PostAppendVar(TArray<TUniquePtr<SpvInstruction>>& InstList, SpvPointer* Pointer, SpvId PackedHeader, SpvId VarId) override;
		void PostAppendCall(TArray<TUniquePtr<SpvInstruction>>& InstList, SpvId PackedHeader, SpvId CallId) override;
		void PostCallReturn(TArray<TUniquePtr<SpvInstruction>>& InstList, SpvId CallId) override;

	public:
		void Visit(const SpvOpPhi* Inst) override;

	private:
		//Create the _PreviewerParams_ uniform buffer (contains uint TargetIteration, TargetPackedHeader, TargetVarId)
		SpvId PatchPreviewerParams();
		//Find the SpvStorageClass::Output variable with Location=0
		SpvId FindOutputVariable();
		//Generate SPIR-V instructions to convert a value of SrcType to float4
		void ConvertToFloat4(SpvType* SrcType, SpvId SrcValue, TArray<TUniquePtr<SpvInstruction>>& InstList, SpvId& OutFloat4Value);
		//Convert a scalar value to float
		void ConvertScalarToFloat(SpvType* SrcType, SpvId SrcValue, TArray<TUniquePtr<SpvInstruction>>& InstList, SpvId& OutFloatValue);

	private:
		SpvId PreviewOutputVar;   // float4 Private global
		SpvId StateCounter;       // uint Private global - counts how many times target (PackedHeader, VarId, CallStackHash) matched
		SpvId CallStackHashVar;   // uint Private global - hash of the current call stack
		SpvId PreviewerParams;    // uniform buffer with TargetIteration, TargetPackedHeader, TargetVarId, TargetCallStackHash
		SpvId OutputVarId;        // the original Output variable (fragColor/outColor)
		TMap<SpvId, SpvId> BlockSplitRemaps; // original block label -> last continuation label after PostAppendVar split
	};
}
