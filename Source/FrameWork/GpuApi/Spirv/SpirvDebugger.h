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
		FuncCall,

		//Tag
		Condition,
		FuncCallAfterReturn,
		Return,
		Kill,

		//UBSan
		Normalize,
		SmoothStep,
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

	struct SpvDebugState_FuncCall
	{
		int32 Line{};
		SpvId CallId;
	};

	struct SpvDebugState_Tag
	{
		int32 Line{};
		bool bFuncCallAfterReturn : 1 {};
		bool bReturn : 1 {};
		bool bCondition : 1 {};
		bool bKill : 1{};
	};

	struct SpvDebugState_Normalize
	{
		int32 Line{};
		SpvId ResultType;
		TArray<uint8> X;
	};
	struct SpvDebugState_SmoothStep
	{
		int32 Line{};
		SpvId ResultType;
		TArray<uint8> Edge0;
		TArray<uint8> Edge1;
	};

	using SpvDebugState = std::variant<SpvDebugState_VarChange, SpvDebugState_ScopeChange, SpvDebugState_ReturnValue, SpvDebugState_FuncCall, SpvDebugState_Tag,
		SpvDebugState_Normalize, SpvDebugState_SmoothStep>;

	struct SpvBinding
	{
		int32 DescriptorSet;
		int32 Binding;
		
		TRefCountPtr<GpuResource> Resource;
	};

	struct SpvFuncCall
	{
		SpvId Callee;
		TArray<SpvId> Arguments;
	};

	struct SpvFunc
	{
		SpvId ReturnType;
		TArray<SpvId> Parameters;
	};

	struct SpvBasicBlock
	{
		TArray<int32> ValidLines;
	};

	struct SpvDebuggerContext : SpvMetaContext
	{
		TArray<SpvBinding> Bindings;
		std::unordered_map<SpvId, SpvPointer> LocalPointers;
		std::unordered_map<SpvId, SpvVariable> LocalVariables;
		std::unordered_map<SpvId, SpvFuncCall> FuncCalls;
		std::unordered_map<SpvId, SpvFunc> Funcs;
		std::unordered_map<SpvId, SpvBasicBlock> BBs;

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

		bool IsParameter(SpvId Id)
		{
			for (const auto& [_, Func] : Funcs)
			{
				if (Func.Parameters.Contains(Id))
				{
					return true;
				}
			}
			return false;
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
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections, const TMap<SpvId, SpvExtSet>& InExtSets) override;
		int32 GetInstIndex(SpvId Inst) const;
		
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
		virtual void PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList) = 0;
		virtual void ParseInternal();
		void PatchToDebugger(SpvId InValueId, SpvId InTypeId, TArray<TUniquePtr<SpvInstruction>>& InstList);
		void PatchAppendVarFunc(SpvPointer* Pointer, uint32 IndexNum);
		void PatchAppendValueFunc(SpvType* ValueType);
		void PatchAppendMathFunc(SpvType* ResultType, uint32 OperandNum);
		void AppendScope(const TFunction<int32()>& OffsetEval);
		void AppendTag(const TFunction<int32()>& OffsetEval, SpvDebuggerStateType InStateType);
		void AppendValue(const TFunction<int32()>& OffsetEval, SpvType* ValueType, SpvId Value, SpvDebuggerStateType InStateType);
		void AppendMath(const TFunction<int32()>& OffsetEval, SpvType* ResultType, const TArray<SpvId>& Operands, SpvDebuggerStateType InStateType);

	protected:
		SpvDebuggerContext& Context;
		int32 InstIndex;
		SpvLexicalScope* CurScope = nullptr;
		SpvBasicBlock* CurBlock = nullptr;
		int32 CurLine{};
		SpvFunc* CurFunc{};

		bool EnableUbsan;
		
		const TArray<TUniquePtr<SpvInstruction>>* Insts;
		SpvId DebuggerBuffer;
		SpvId DebuggerOffset;
		SpvId AppendScopeFuncId, AppendTagFuncId;
		SpvId AppendCallFuncId;
		TMap<TPair<SpvType*, uint32>, SpvId> AppendVarFuncIds;
		TMap<SpvType*, SpvId> AppendValueFuncIds;
		TMap<TPair<SpvType*, uint32>, SpvId> AppendMathFuncIds;
		TMap<const SpvOpFunctionCall*, const SpvOpStore*> AppendVarCallToStore;
		SpvPatcher Patcher;
	};

	FRAMEWORK_API std::tuple<SpvType*, int32> GetAccess(const SpvVariable* Var, const TArray<uint32>& Indexes);

}
