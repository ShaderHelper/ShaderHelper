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
		void Visit(const SpvFClamp* Inst) override;
		void Visit(const SpvUClamp* Inst) override;
		void Visit(const SpvSClamp* Inst) override;
		void Visit(const SpvSmoothStep* Inst) override;
		void Visit(const SpvOpConvertFToU* Inst) override;
		void Visit(const SpvOpConvertFToS* Inst) override;
		void Visit(const SpvOpUDiv* Inst) override;
		void Visit(const SpvOpSDiv* Inst) override;
		void Visit(const SpvOpUMod* Inst) override;
		void Visit(const SpvOpSRem* Inst) override;
		void Visit(const SpvOpFRem* Inst) override;

	protected:
		void PatchAppendErrorFunc();
		void AppendError(TArray<TUniquePtr<SpvInstruction>>& InstList);
		void AppendAnyEqualZeroError(const TFunction<int32()>& OffsetEval, SpvId ResultTypeId, SpvId ValueId);
		SpvId GetScalarOrVectorTypeId(SpvType* ResultType, SpvId ScalarTypeId);
		template<typename T>
		SpvId GetScalarOrVectorId(SpvType* ResultType, T ScalarValue)
		{
			SpvId ScalarId = Patcher.FindOrAddConstant(ScalarValue);
			if (SpvVectorType* VecType = dynamic_cast<SpvVectorType*>(ResultType))
			{
				uint32 ElemCount = VecType->ElementCount;
				SpvId VecTypeId = Patcher.FindOrAddType(MakeUnique<SpvOpTypeVector>(VecType->ElementType->GetId(), ElemCount));
				TArray<SpvId> Constituents;
				Constituents.Init(ScalarId, ElemCount);
				return Patcher.FindOrAddConstant(MakeUnique<SpvOpConstantComposite>(VecTypeId, Constituents));
			}
			else
			{
				return ScalarId;
			}
			
		}

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
