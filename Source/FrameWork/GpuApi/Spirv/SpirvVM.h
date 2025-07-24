#pragma once
#include "SpirvParser.h"
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
		FString UbError;
	};

	struct SpvRecordedInfo
	{
		std::unordered_map<SpvId, SpvVariable> AllVariables;
		TArray<SpvDebugState> DebugStates;
	};

	struct SpvVmBinding
	{
		int32 DescriptorSet;
		int32 Binding;
		
		TRefCountPtr<GpuResource> Resource;
	};

	struct SpvVmFrame
	{
		int32 ReturnPointIndex{};
		std::queue<SpvPointer*> Arguments;
		SpvObject* ReturnObject = nullptr;
		SpvId CurBasicBlock{};
		SpvId PreBasicBlock{};
		SpvLexicalScope* CurScope = nullptr;
		int32 CurLine{};
		std::unordered_map<SpvId, SpvObject> IntermediateObjects;
		std::unordered_map<SpvId, SpvPointer> Pointers;
		std::unordered_map<SpvId, SpvVariable> Variables;
	};

	struct SpvThreadState
	{
		int32 InstIndex;
		int32 NextInstIndex;
		//Can not use TArray<SpvVmFrame>, TArray requires the trivially relocatable type.
		//However, std::unordered_map may not be trivially relocatable.
		std::vector<SpvVmFrame> StackFrames;
		
		std::unordered_map<SpvBuiltIn, TArray<uint8>> BuiltInInput;
		std::unordered_map<uint32, TArray<uint8>> LocationInput;
		
		//Finalized information for external use
		SpvRecordedInfo RecordedInfo;
	};

	struct SpvVmContext : SpvMetaContext
	{
		SpvThreadState ThreadState;
	};

	class FRAMEWORK_API SpvVmVisitor : public SpvVisitor
	{
	public:
		SpvVmVisitor() = default;
		virtual SpvVmContext& GetActiveContext() = 0;
		
	public:
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts) override;
		int32 GetInstIndex(SpvId Inst) const;
		SpvPointer* GetPointer(SpvId PointerId);
		TArray<uint8> ExecuteGpuOp(const FString& Name, int32 ResultSize, const TArray<uint8>& InputData, const TArray<FString>& Args);
		
	public:
		void Visit(SpvDebugLine* Inst) override;
		void Visit(SpvDebugScope* Inst) override;
		void Visit(SpvDebugDeclare* Inst) override;
		
		void Visit(SpvSin* Inst) override;
		void Visit(SpvCos* Inst) override;
		void Visit(SpvPow* Inst) override;
		
		void Visit(SpvOpFunctionParameter* Inst) override;
		void Visit(SpvOpFunctionCall* Inst) override;
		void Visit(SpvOpVariable* Inst) override;
		void Visit(SpvOpLabel* Inst) override;
		void Visit(SpvOpLoad* Inst) override;
		void Visit(SpvOpStore* Inst) override;
		void Visit(SpvOpVectorShuffle* Inst) override;
		void Visit(SpvOpCompositeConstruct* Inst) override;
		void Visit(SpvOpCompositeExtract* Inst) override;
		void Visit(SpvOpAccessChain* Inst) override;
		void Visit(SpvOpVectorTimesScalar* Inst) override;
		void Visit(SpvOpIsNan* Inst) override;
		void Visit(SpvOpSelect* Inst) override;
		void Visit(SpvOpIEqual* Inst) override;
		void Visit(SpvOpINotEqual* Inst) override;
		void Visit(SpvOpSGreaterThan* Inst) override;
		void Visit(SpvOpSLessThan* Inst) override;
		void Visit(SpvOpFOrdLessThan* Inst) override;
		void Visit(SpvOpFOrdGreaterThan* Inst) override;
		void Visit(SpvOpConvertFToS* Inst) override;
		void Visit(SpvOpConvertSToF* Inst) override;
		void Visit(SpvOpFNegate* Inst) override;
		void Visit(SpvOpIAdd* Inst) override;
		void Visit(SpvOpFAdd* Inst) override;
		void Visit(SpvOpISub* Inst) override;
		void Visit(SpvOpFSub* Inst) override;
		void Visit(SpvOpIMul* Inst) override;
		void Visit(SpvOpFMul* Inst) override;
		void Visit(SpvOpFDiv* Inst) override;
		void Visit(SpvOpBitwiseAnd* Inst) override;
		void Visit(SpvOpBranch* Inst) override;
		void Visit(SpvOpBranchConditional* Inst) override;
		void Visit(SpvOpReturn* Inst) override;
		void Visit(SpvOpReturnValue* Inst) override;

	protected:
		//if there is an ub case, the debugger results may not match gpu
		bool EnableUbsan = true;
		
		bool bTerminate{};
		const TArray<TUniquePtr<SpvInstruction>>* Insts;
	};

	SpvObject* GetObject(SpvVmContext* InContext, SpvId ObjectId);
	TArray<uint8> GetPointerValue(SpvVmContext* InContext, SpvPointer* InPointer);
	TArray<uint8> GetObjectValue(SpvObject* InObject, const TArray<uint32>& Indexes = {});
	void WritePointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc, const TArray<uint8>& ValueToStore, SpvVariableChange* OutVariableChange = nullptr);
}
