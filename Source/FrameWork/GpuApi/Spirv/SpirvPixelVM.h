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
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts) override;
		void ParseQuad(int32 QuadIndex, int32 StopIndex = INDEX_NONE);
		
		void Visit(SpvOpDPdx* Inst) override;
		
	protected:
		SpvVmPixelContext& PixelContext;
	};
}
