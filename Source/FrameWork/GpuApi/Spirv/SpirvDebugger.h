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
		Access,
		Normalize,
		SmoothStep,
		Pow,
		Clamp,
		Div,
		ConvertF,
		Remainder,
	};

	struct SpvDebugState_VarChange
	{
		int32 Line{};
		SpvVarChange Change;
		FString Error;
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


	struct SpvDebugState_Access
	{
		int32 Line{};
		SpvId VarId;
		TArray<int32> Indexes;
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
	struct SpvDebugState_Pow
	{
		int32 Line{};
		SpvId ResultType;
		TArray<uint8> X;
		TArray<uint8> Y;
	};
	struct SpvDebugState_Clamp
	{
		int32 Line{};
		SpvId ResultType;
		TArray<uint8> MinVal;
		TArray<uint8> MaxVal;
	};
	struct SpvDebugState_Div
	{
		int32 Line{};
		SpvId ResultType;
		TArray<uint8> Operand2;
	};
	struct SpvDebugState_ConvertF
	{
		int32 Line{};
		SpvId ResultType;
		TArray<uint8> FloatValue;
	};
	struct SpvDebugState_Remainder
	{
		int32 Line{};
		SpvId ResultType;
		TArray<uint8> Operand1;
		TArray<uint8> Operand2;
	};

	using SpvDebugState = std::variant<SpvDebugState_VarChange, SpvDebugState_ScopeChange, SpvDebugState_ReturnValue, SpvDebugState_FuncCall, SpvDebugState_Tag,
		SpvDebugState_Access, SpvDebugState_Normalize, SpvDebugState_SmoothStep, SpvDebugState_Pow, SpvDebugState_Clamp, SpvDebugState_Div, SpvDebugState_ConvertF, 
		SpvDebugState_Remainder>;

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
		SpvId Type;
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
		virtual void PatchActiveCondition(TArray<TUniquePtr<SpvInstruction>>& InstList) {}
		virtual void ParseInternal();
		void PatchToDebugger(SpvId InValueId, SpvId InTypeId, TArray<TUniquePtr<SpvInstruction>>& InstList);
		void PatchAppendAccessFunc(int32 IndexNum);
		void PatchAppendVarFunc(SpvPointer* Pointer, int32 IndexNum);
		void PatchAppendValueFunc(SpvType* ValueType);
		void PatchAppendMathFunc(SpvType* ResultType, SpvType* OperandType, int32 OperandNum);
		SpvOpFunctionCall* AppendVar(const TFunction<int32()>& OffsetEval, SpvPointer* Pointer);
		void AppendScope(const TFunction<int32()>& OffsetEval);
		void AppendAccess(const TFunction<int32()>& OffsetEval, SpvPointer* Pointer);
		void AppendTag(const TFunction<int32()>& OffsetEval, SpvDebuggerStateType InStateType);
		void AppendValue(const TFunction<int32()>& OffsetEval, SpvType* ValueType, SpvId Value, SpvDebuggerStateType InStateType);
		void AppendMath(const TFunction<int32()>& OffsetEval, SpvType* ResultType, SpvType* OperandType, const TArray<SpvId>& Operands, SpvDebuggerStateType InStateType);

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
		TMap<TPair<SpvType*, int32>, SpvId> AppendVarFuncIds;
		TMap<SpvType*, SpvId> AppendValueFuncIds;
		TMap<int32, SpvId> AppendAccessFuncIds;
		struct MathFuncSearchKey
		{
			SpvType* ResultType{};
			SpvType* OperandType{};
			int32 OperandNum{};
			bool operator==(const MathFuncSearchKey& Other) const = default;
			friend uint32 GetTypeHash(const MathFuncSearchKey& Key)
			{
				uint32 Hash = HashCombine(::GetTypeHash(Key.ResultType), ::GetTypeHash(Key.OperandType));
				Hash = HashCombine(Hash, ::GetTypeHash(Key.OperandNum));
				return Hash;
			}
		};
		TMap<MathFuncSearchKey, SpvId> AppendMathFuncIds;
		TMap<const SpvOpFunctionCall*, const SpvOpStore*> AppendVarCallToStore;
		SpvPatcher Patcher;
	};

	FRAMEWORK_API TValueOrError<std::tuple<SpvType*, int32>, FString> GetAccess(const SpvVariable* Var, const TArray<int32>& Indexes);
	SpvId PatchDebuggerBuffer(SpvPatcher& Patcher);
	int32 GetInstIndex(const TArray<TUniquePtr<SpvInstruction>>* Insts, SpvId Inst);

}
