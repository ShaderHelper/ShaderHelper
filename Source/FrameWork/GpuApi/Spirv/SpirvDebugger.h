#pragma once
#include "SpirvParser.h"
#include "SpirvPatcher.h"
#include <queue>

namespace FW
{
	struct SpvVariableChange
	{
		SpvId VarId;
		TArray<uint8> PreValue;
		TArray<uint8> NewValue;
		struct DirtyRange
		{
			int32 OffsetBytes;
			int32 ByteSize;
		} Range ;
	};

	struct SpvLexicalScopeChange
	{
		SpvLexicalScope* PreScope{};
		SpvLexicalScope* NewScope{};
	};

	struct SpvDebugState
	{
		int32 Line{};
		std::optional<SpvLexicalScopeChange> ScopeChange;
		TArray<SpvVariableChange> VarChanges;
		std::optional<SpvObject> ReturnObject;
		bool bFuncCall : 1 {};
		bool bFuncCallAfterReturn : 1 {};
		bool bReturn : 1 {};
		bool bCondition : 1 {};
		bool bParamChange : 1 {};
		bool bKill : 1{};
		FString UbError;
	};

	struct SpvBinding
	{
		int32 DescriptorSet;
		int32 Binding;
		
		TRefCountPtr<GpuResource> Resource;
	};

	struct SpvDebuggerContext : SpvMetaContext
	{
		TArray<SpvBinding> Bindings;

		int32 InstIndex;

		SpvLexicalScope* Scope = nullptr;
		int32 Line{};

		std::unordered_map<SpvId, SpvVariable> LocalVariables;

		SpvVariable* FindVar(SpvId Id)
		{
			if (GlobalVariables.contains(Id))
			{
				return &GlobalVariables[Id];
			}
			else if(LocalVariables.contains(Id))
			{
				return &LocalVariables[Id];
			}
			return nullptr;
		}
	};

	class FRAMEWORK_API SpvDebuggerVisitor : public SpvVisitor
	{
	public:
		SpvDebuggerVisitor() = default;
		SpvDebuggerVisitor(SpvDebuggerContext& InContext) : Context(InContext)
		{
		}
		
	public:
		const SpvPatcher& GetPatcher() const { return Patcher; }
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections) override;
		int32 GetInstIndex(SpvId Inst) const;
		virtual void PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList) = 0;
		
	public:
		void Visit(const SpvDebugLine* Inst) override;
		void Visit(const SpvDebugScope* Inst) override;
		void Visit(const SpvDebugDeclare* Inst) override;
		void Visit(const SpvDebugValue* Inst) override;

		void Visit(const SpvPow* Inst) override;
		void Visit(const SpvFClamp* Inst) override;
		void Visit(const SpvUClamp* Inst) override;
		void Visit(const SpvSClamp* Inst) override;
		void Visit(const SpvSmoothStep* Inst) override;
		void Visit(const SpvNormalize* Inst) override;
		
		void Visit(const SpvOpConvertFToU* Inst) override;
		void Visit(const SpvOpConvertFToS* Inst) override;
		void Visit(const SpvOpUDiv* Inst) override;
		void Visit(const SpvOpSDiv* Inst) override;
		void Visit(const SpvOpUMod* Inst) override;
		void Visit(const SpvOpSRem* Inst) override;
		void Visit(const SpvOpFRem* Inst) override;

		void Visit(const SpvOpFunctionCall* Inst) override;
		void Visit(const SpvOpVariable* Inst) override;
		void Visit(const SpvOpPhi* Inst) override;
		void Visit(const SpvOpLabel* Inst) override;
		void Visit(const SpvOpLoad* Inst) override;
		void Visit(const SpvOpStore* Inst) override;
		void Visit(const SpvOpIEqual* Inst) override;
		void Visit(const SpvOpINotEqual* Inst) override;
		void Visit(const SpvOpUGreaterThan* Inst) override;
		void Visit(const SpvOpSGreaterThan* Inst) override;
		void Visit(const SpvOpUGreaterThanEqual* Inst) override;
		void Visit(const SpvOpSGreaterThanEqual* Inst) override;
		void Visit(const SpvOpULessThan* Inst) override;
		void Visit(const SpvOpSLessThan* Inst) override;
		void Visit(const SpvOpULessThanEqual* Inst) override;
		void Visit(const SpvOpSLessThanEqual* Inst) override;
		void Visit(const SpvOpFOrdEqual* Inst) override;
		void Visit(const SpvOpFOrdNotEqual* Inst) override;
		void Visit(const SpvOpFOrdLessThan* Inst) override;
		void Visit(const SpvOpFOrdGreaterThan* Inst) override;
		void Visit(const SpvOpFOrdLessThanEqual* Inst) override;
		void Visit(const SpvOpFOrdGreaterThanEqual* Inst) override;

		void Visit(const SpvOpBranch* Inst) override;
		void Visit(const SpvOpBranchConditional* Inst) override;
		void Visit(const SpvOpReturn* Inst) override;
		void Visit(const SpvOpReturnValue* Inst) override;

	protected:
		SpvDebuggerContext& Context;

		bool EnableUbsan = true;
		
		const TArray<TUniquePtr<SpvInstruction>>* Insts;
		SpvId DebuggerBuffer;
		SpvId DebuggerStateType;
		SpvId DebuggerOffset;
		SpvId DebuggerLine;
		SpvId DebuggerVarId, DebuggerVarDirtyOffset, DebuggerVarDirtyByteSize;
		SpvId DebuggerScopeId;
		SpvId AppendScopeFuncId;
		SpvPatcher Patcher;
	};

	TArray<uint8> GetPointerValue(SpvDebuggerContext* InContext, SpvPointer* InPointer);
	TArray<uint8> GetObjectValue(SpvObject* InObject, const TArray<uint32>& Indexes = {}, int32* OutOffset = nullptr);
	void WritePointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc, const TArray<uint8>& ValueToStore, SpvVariableChange* OutVariableChange = nullptr);
}
