#pragma once
#include "SpirvPixelVM.h"

namespace FW
{
	struct SpvVmPixelExprContext : SpvVmPixelContext
	{
		//Out
		SpvTypeDesc* ResultTypeDesc{};
		TArray<uint8> ResultValue;
		bool HasSideEffect{};
	};

	class FRAMEWORK_API SpvVmPixelExprVisitor : public SpvVmPixelVisitor
	{
	public:
		SpvVmPixelExprVisitor(SpvVmPixelExprContext& InPixelExprContext);
		
	public:
		void Visit(SpvDebugDeclare* Inst) override;
		void Visit(SpvOpStore* Inst) override;
		
	protected:
		SpvVmPixelExprContext& PixelExprContext;
	};
}
