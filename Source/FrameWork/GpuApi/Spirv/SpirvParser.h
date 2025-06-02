#pragma once
#include "SpirvInstruction.h"

namespace FW
{
	//Info before logical layout section 10
	struct SpvMetaContext
	{
		Vector3u ThreadGroupSize{};
		TMap<SpvId, FString> DebugStrs;
		TMap<SpvId, SpvType*> Types;
	};

	class SpvMetaVisitor : public SpvVisitor
	{
	public:
		SpvMetaVisitor(SpvMetaContext& InContext) : Context(InContext)
		{}
		
	public:
		void Visit(SpvOpTypeFloat* Inst) override;
		
		void Visit(SpvOpExecutionMode* Inst) override;
		void Visit(SpvOpString* Inst) override;
		void Visit(SpvOpConstant* Inst) override;
	
	private:
		SpvMetaContext& Context;
	};

	void ParseSpv(const TArray<uint32>& SpvCode, const TArray<SpvVisitor*>& Visitors, FString& OutErrorInfo);
}
