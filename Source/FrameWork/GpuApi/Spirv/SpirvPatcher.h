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
		const TArray<TUniquePtr<SpvInstruction>>& GetPathcedInsts() const { return PatchedInsts; }
		FString GetAsm() const;
		void Dump(const FString& SavedFileName) const;
		SpvId NewId() {
			return SpvCode[3]++;
		}

		//The result ID is inferred internally
		SpvId FindOrAddType(TUniquePtr<SpvInstruction> InInst);
		template<typename T>
		requires requires { T{} + T{}; }
		SpvId FindOrAddConstant(T InConstant);
		SpvId FindOrAddConstant(TUniquePtr<SpvInstruction> InInst);
		SpvId FindOrAddTypeDesc(TUniquePtr<SpvInstruction> InInst);
		SpvId FindOrAddDebugStr(const FString& Str);

		void AddDebugName(TUniquePtr<SpvInstruction> InInst);
		void AddAnnotation(TUniquePtr<SpvInstruction> InInst);
		void AddGlobalVariable(TUniquePtr<SpvInstruction> InInst);
		void AddFunction(TArray<TUniquePtr<SpvInstruction>>&& Function);

		void AddInstructions(int WordOffset, TArray<TUniquePtr<SpvInstruction>>&& InInsts);
		void AddInstruction(int WordOffset, TUniquePtr<SpvInstruction> InInst);

		void OverwriteInstruction(int WordOffset, int OldWordLen, TUniquePtr<SpvInstruction> NewInst);
		void OverwriteWord(int WordOffset, uint32 NewWord);

	private:
		void UpdateSection(SpvSectionKind DirtySection, int WordSize);
		void UpdateInsts(int WordOffset, int WordSize);

	private:
		const TArray<TUniquePtr<SpvInstruction>>* OriginInsts;
		TArray<TUniquePtr<SpvInstruction>> PatchedInsts;
		TArray<uint32> SpvCode;
		TUniquePtr<SpvMetaVisitor> MetaVisitor;

		// Sorted indices for efficient offset updates (binary search)
		TArray<SpvInstruction*> SortedOriginInsts;
		TArray<SpvInstruction*> SortedPatchedInsts;
	};
}
