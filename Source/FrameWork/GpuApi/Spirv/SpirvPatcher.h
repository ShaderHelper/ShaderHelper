#pragma once
#include "SpirvInstruction.h"

namespace FW
{
	struct SpvMetaContext;
	class SpvMetaVisitor;

	class FRAMEWORK_API SpvPatcher
	{
	public:
		void SetSpvContext(const TArray<uint32>& InSpvCode, SpvMetaContext* InMetaContext);
		const TArray<uint32>& GetSpv() const { return SpvCode; }
		void Dump(const FString& SavedFileName) const;
		SpvId NewId() {
			return SpvCode[3]++;
		}

		void AddAnnotation(const SpvInstruction* Inst);
		void AddType(const SpvInstruction* Inst);
		void AddConstant(const SpvInstruction* Inst);
		void AddGlobalVariable(const SpvInstruction* Inst);

	private:
		void UpdateSection(SpvSectionKind DirtySection, int WordSize);

	private:
		TArray<uint32> SpvCode;
		TUniquePtr<SpvMetaVisitor> MetaVisitor;
	};
}
