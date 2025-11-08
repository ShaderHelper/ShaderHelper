#pragma once
#include "SpirvInstruction.h"

namespace FW
{
	struct SpvMetaContext;
	class SpvMetaVisitor;

	class FRAMEWORK_API SpvPatcher
	{
	public:
		void SetSpvContext(const TArray<TUniquePtr<SpvInstruction>>& InInsts, const TArray<uint32>& InSpvCode, SpvMetaContext* InMetaContext);
		const TArray<uint32>& GetSpv() const { return SpvCode; }
		void Dump(const FString& SavedFileName) const;
		SpvId NewId() {
			return SpvCode[3]++;
		}

		//The result ID is inferred internally
		SpvId FindOrAddType(TUniquePtr<SpvInstruction> InInst);
		template<typename T>
		SpvId FindOrAddConstant(T InConstant);

		void AddDebugName(TUniquePtr<SpvInstruction> InInst);
		void AddAnnotation(TUniquePtr<SpvInstruction> InInst);
		void AddGlobalVariable(TUniquePtr<SpvInstruction> InInst);
		void AddFunction(TArray<TUniquePtr<SpvInstruction>>&& Function);

		void AddInstructions(int WordOffset, TArray<TUniquePtr<SpvInstruction>>&& InInsts);
		void AddInstruction(int WordOffset, TUniquePtr<SpvInstruction> InInst);

	private:
		void UpdateSection(SpvSectionKind DirtySection, int WordSize);
		void UpdateOriginInsts(int WordOffset, int WordSize);

	private:
		const TArray<TUniquePtr<SpvInstruction>>* OriginInsts;
		TArray<TUniquePtr<SpvInstruction>> PatchedInsts;
		TArray<uint32> SpvCode;
		TUniquePtr<SpvMetaVisitor> MetaVisitor;
	};
}
