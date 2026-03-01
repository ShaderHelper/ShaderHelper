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
	//A float4 global _PreviewOutput_ is assigned when PackedHeader and VarId match
	//the uniform target, and the per-match iteration counter equals TargetIteration.
	//The counter only increments for the target (PackedHeader, VarId) pair,
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
		SpvId StateCounter;       // uint Private global - counts how many times target (PackedHeader, VarId) matched
		SpvId PreviewerParams;    // uniform buffer with TargetIteration, TargetPackedHeader, TargetVarId
		SpvId OutputVarId;        // the original Output variable (fragColor/outColor)
	};
}
