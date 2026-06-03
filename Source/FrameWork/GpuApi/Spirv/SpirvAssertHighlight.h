#pragma once
#include "SpirvDebugger.h"

namespace FW
{
	class FRAMEWORK_API SpvAssertHighlightVisitor : public SpvVisitor
	{
	public:
		SpvAssertHighlightVisitor(SpvMetaContext& InContext, ShaderType InStage, TArray<SpvBinding> InRuntimeBindings = {})
			: Context(InContext), Stage(InStage)
			, RuntimeBindings(MoveTemp(InRuntimeBindings))
		{}

		const SpvPatcher& GetPatcher() const { return Patcher; }

		void Parse(TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections) override;

	private:
		SpvId FindAssertResultVar() const;
		void CollectLocationOutputs(TArray<SpvId>& OutOutputs) const;
		// Returns 0 if the Output's pointee type cannot represent pink (skip writing).
		SpvId BuildPinkConstantForType(SpvType* PointeeType);
		void PatchAssertTerminator(SpvInstruction* Term, void (SpvAssertHighlightVisitor::*AppendFailBody)(TArray<TUniquePtr<SpvInstruction>>&));

		// Pixel path
		void PatchPixelTerminator(SpvInstruction* Term);
		void AppendPixelFailBranch(TArray<TUniquePtr<SpvInstruction>>& InstList);

		// Compute path
		SpvId DeclareAssertThreadBuffer();
		void PatchComputeTerminator(SpvInstruction* Term);
		void AppendComputeFailReport(TArray<TUniquePtr<SpvInstruction>>& InstList);

		// Vertex path
		SpvId DeclareAssertVertexBuffer();
		void PatchVertexTerminator(SpvInstruction* Term);
		void AppendVertexFailReport(TArray<TUniquePtr<SpvInstruction>>& InstList);

	private:
		SpvMetaContext& Context;
		ShaderType Stage;
		TArray<SpvBinding> RuntimeBindings;
		SpvPatcher Patcher;
		const TArray<TUniquePtr<SpvInstruction>>* Insts = nullptr;
		SpvId AssertResultVar;
		TArray<SpvId> LocationOutputs;
		SpvId AssertThreadBufferVar;
		SpvId GlobalInvocationIdVar;
		SpvId AssertVertexBufferVar;
		SpvId VertexIndexVar;
		SpvId InstanceIndexVar;
	};

	// Assert-highlight injects RWByteAddressBuffers at these fixed set/bindings.
	inline constexpr int AssertHighlightBindGroupSlot = 7;
	inline constexpr int AssertThreadBufferBindingSlot = 0731;
	inline constexpr int AssertVertexBufferBindingSlot = 0732;
}
