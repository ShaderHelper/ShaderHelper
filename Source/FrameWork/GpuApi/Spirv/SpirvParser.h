#pragma once
#include "SpirvCommon.h"
#include "SpirvInstruction.h"

namespace FW
{
	struct SpvMetaContext
	{
		Vector3u ThreadGroupSize{};
	};

	class SpvMetaVisitor : public SpvVisitor
	{
	public:
		SpvMetaVisitor(SpvMetaContext& InContext) : Context(InContext)
		{}
		
	public:
		void Visit(SpvExecutionModeOp* Inst) override;
	
	private:
		SpvMetaContext& Context;
	};

	void ParseSpv(const TArray<uint32>& SpvCode, const TArray<SpvVisitor*>& Visitors, FString& OutErrorInfo);
}
