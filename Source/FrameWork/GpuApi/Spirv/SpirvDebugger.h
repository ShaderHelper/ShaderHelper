#pragma once
#include "SpirvParser.h"
#include "SpirvPatcher.h"
#include <queue>

namespace FW
{
	struct SpvVarDirtyRange
	{
		int32 ByteOffset;
		int32 ByteSize;
	} ;
	struct SpvVarChange
	{
		SpvId VarId;
		TArray<uint8> PreDirtyValue;
		TArray<uint8> NewDirtyValue;
		int32 ByteOffset;
	};

	struct SpvScopeChange
	{
		SpvLexicalScope* PreScope{};
		SpvLexicalScope* NewScope{};
	};

	enum class SpvDebuggerStateType : uint32
	{
		None,
		VarChange,
		ScopeChange,
		ReturnValue,

		Condition,
		FuncCall,
		FuncCallAfterReturn,
		Return,
		Kill,
	};

	struct SpvDebugState_VarChange
	{
		int32 Line{};
		SpvVarChange Change;
	};

	struct SpvDebugState_ScopeChange
	{
		SpvScopeChange Change;
	};

	struct SpvDebugState_ReturnValue
	{
		int32 Line{};
		TArray<uint8> Value;
	};

	struct SpvDebugState_Tag
	{
		int32 Line{};
		bool bFuncCall : 1 {};
		bool bFuncCallAfterReturn : 1 {};
		bool bReturn : 1 {};
		bool bCondition : 1 {};
		bool bKill : 1{};
	};

	using SpvDebugState = std::variant<SpvDebugState_VarChange, SpvDebugState_ScopeChange, SpvDebugState_ReturnValue, SpvDebugState_Tag>;

	struct SpvBinding
	{
		int32 DescriptorSet;
		int32 Binding;
		
		TRefCountPtr<GpuResource> Resource;
	};

	struct SpvDebuggerContext : SpvMetaContext
	{
		//Can not obtain information from spirv to distinguish whether formal paramaters have special semantics such as out/inout，
		//Therefore, obtain it from the editor
		TArray<ShaderFunc> EditorFuncInfo;

		TArray<SpvBinding> Bindings;
		std::unordered_map<SpvId, SpvPointer> LocalPointers;
		std::unordered_map<SpvId, SpvVariable> LocalVariables;
		std::unordered_map<SpvId, SpvVariable*> FuncParameters;

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

		SpvPointer* FindPointer(SpvId Id)
		{
			if (GlobalPointers.contains(Id))
			{
				return &GlobalPointers[Id];
			}
			else if (LocalPointers.contains(Id))
			{
				return &LocalPointers[Id];
			}
			return nullptr;
		}
	};

	class FRAMEWORK_API SpvDebuggerVisitor : public SpvVisitor
	{
	public:
		SpvDebuggerVisitor() = default;
		SpvDebuggerVisitor(SpvDebuggerContext& InContext, bool InEnableUbsan) 
			: Context(InContext)
			, EnableUbsan(InEnableUbsan)
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
		void Visit(const SpvDebugFunctionDefinition* Inst) override;

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

		void Visit(const SpvOpFunction* Inst) override;
		void Visit(const SpvOpFunctionCall* Inst) override;
		void Visit(const SpvOpFunctionParameter* Inst) override;
		void Visit(const SpvOpVariable* Inst) override;
		void Visit(const SpvOpPhi* Inst) override;
		void Visit(const SpvOpLabel* Inst) override;
		void Visit(const SpvOpLoad* Inst) override;
		void Visit(const SpvOpStore* Inst) override;
		void Visit(const SpvOpAccessChain* Inst) override;
		void Visit(const SpvOpSwitch* Inst) override;
		void Visit(const SpvOpBranch* Inst) override;
		void Visit(const SpvOpBranchConditional* Inst) override;
		void Visit(const SpvOpReturn* Inst) override;
		void Visit(const SpvOpReturnValue* Inst) override;
	protected:
		void PatchToDebugger(SpvId InValueId, SpvId InTypeId, TArray<TUniquePtr<SpvInstruction>>& InstList);
		void PatchAppendVarFunc(SpvPointer* Pointer, uint32 IndexNum);
		void PatchAppendValueFunc(SpvType* ValueType);

	protected:
		SpvDebuggerContext& Context;
		int32 InstIndex;
		SpvLexicalScope* CurScope = nullptr;
		int32 CurLine{};
		TArray<SpvVariable*> CurFuncParams;
		SpvType* CurReturnType;

		bool EnableUbsan;
		
		const TArray<TUniquePtr<SpvInstruction>>* Insts;
		SpvId DebuggerBuffer;
		SpvId DebuggerOffset;
		SpvId AppendScopeFuncId, AppendTagFuncId;
		struct VarFuncSerachKey
		{
			SpvType* Type;
			uint32 IndexNum;
			bool operator==(const VarFuncSerachKey& Other) const = default;
			friend uint32 GetTypeHash(const VarFuncSerachKey& Key)
			{
				return HashCombine(::GetTypeHash(Key.Type), ::GetTypeHash(Key.IndexNum));
			}
		};
		TMap<VarFuncSerachKey, SpvId> AppendVarFuncIds;
		TMap<SpvType*, SpvId> AppendValueFuncIds;
		SpvPatcher Patcher;
	};

	FRAMEWORK_API int32 GetByteOffset(const SpvVariable* Var, const TArray<uint32>& Indexes);

}
