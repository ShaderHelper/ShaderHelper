#pragma once
#include "SpirvVM.h"

namespace FW
{
	struct SpvVmPixelContext
	{
		int32 DebugIndex;
		TArray<SpvVmBinding> Bindings;
		std::array<SpvVmContext, 4> Quad;
		int32 CurActiveIndex;
	};

	class FRAMEWORK_API SpvVmPixelVisitor : public SpvVmVisitor
	{
	public:
		SpvVmPixelVisitor(SpvVmPixelContext& InPixelContext);
		SpvVmContext& GetActiveContext() override;
	public:
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections) override;
		void ParseQuad(int32 QuadIndex, int32 StopIndex = INDEX_NONE);
		bool FlushQuad(int32 InstIndex);
		
		void Visit(const SpvOpDPdx* Inst) override;
		void Visit(const SpvOpDPdy* Inst) override;
		void Visit(const SpvOpFwidth* Inst) override;
		void Visit(const SpvOpImageSampleImplicitLod* Inst) override;
		void Visit(const SpvOpKill* Inst) override;
		
	protected:
		SpvVmPixelContext& PixelContext;
	};
}
