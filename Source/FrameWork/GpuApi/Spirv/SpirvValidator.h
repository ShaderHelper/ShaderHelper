#pragma once
#include "SpirvDebugger.h"

namespace FW
{
	class FRAMEWORK_API SpvValidator : public SpvVisitor
	{
	public:
		SpvValidator(SpvMetaContext& InContext, bool InEnableUbsan, ShaderType InType)
			: Context(InContext), EnableUbsan(InEnableUbsan), Type(InType)
		{}

		const SpvPatcher& GetPatcher() const { return Patcher; }
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections, const TMap<SpvId, SpvExtSet>& InExtSets) override;

	public:
		void Visit(const SpvPow* Inst) override;
		void Visit(const SpvNormalize* Inst) override;

	protected:
		void PatchAppendErrorFunc();
		void AppendError(TArray<TUniquePtr<SpvInstruction>>& InstList);
		SpvId GetBoolTypeId(SpvType* Type);

	private:
		SpvMetaContext& Context;
		bool EnableUbsan;
		ShaderType Type;
		const TArray<TUniquePtr<SpvInstruction>>* Insts;
		SpvId DebuggerBuffer;
		SpvId HasError;
		SpvId AppendErrorFuncId;
		SpvPatcher Patcher;
	};
}
