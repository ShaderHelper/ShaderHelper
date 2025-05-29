#pragma once
#include "SpirvParser.h"

namespace FW
{
	struct SpvVmContext
	{
		SpvMetaContext MetaInfo;
	};

	class SpvVmVisitor : public SpvVisitor
	{
	public:
		SpvVmVisitor(SpvVmContext& InContext) : Context(InContext)
		{}
		
	public:
		void Visit(SpvDebugLine* Inst) override;

	private:
		SpvVmContext& Context;
	};
}
