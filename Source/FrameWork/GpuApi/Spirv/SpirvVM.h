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
		bool bParamChange : 1 {};
		bool bKill : 1{};
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

	struct SpvVmParameter
	{
		SpvPointer* Pointer{};
		ParamSemaFlag Flag{};
	};

	struct SpvVmFrame
	{
		int32 ReturnPointIndex{};
		std::vector<SpvPointer*> Arguments;
		std::vector<SpvVmParameter> Parameters;
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
		//Can not obtain information from spirv to distinguish whether formal paramaters have special semantics such as out/inout，
		//Therefore, obtain it from the editor
		TArray<ShaderFunc> EditorFuncInfo;
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
		TArray<uint8> ExecuteGpuOp(const FString& Name, int32 ResultSize, const TArray<uint8>& InputData, const TArray<FString>& Args, const TArray<GpuResource*>& InResources = {});
		
	public:
		void Visit(SpvDebugLine* Inst) override;
		void Visit(SpvDebugScope* Inst) override;
		void Visit(SpvDebugDeclare* Inst) override;
		void Visit(SpvDebugValue* Inst) override;
		void Visit(SpvDebugFunctionDefinition* Inst) override;
		
		void Visit(SpvRoundEven* Inst) override;
		void Visit(SpvTrunc* Inst) override;
		void Visit(SpvFAbs* Inst) override;
		void Visit(SpvSAbs* Inst) override;
		void Visit(SpvFSign* Inst) override;
		void Visit(SpvSSign* Inst) override;
		void Visit(SpvFloor* Inst) override;
		void Visit(SpvCeil* Inst) override;
		void Visit(SpvFract* Inst) override;
		void Visit(SpvSin* Inst) override;
		void Visit(SpvCos* Inst) override;
		void Visit(SpvTan* Inst) override;
		void Visit(SpvAsin* Inst) override;
		void Visit(SpvAcos* Inst) override;
		void Visit(SpvAtan* Inst) override;
		void Visit(SpvPow* Inst) override;
		void Visit(SpvExp* Inst) override;
		void Visit(SpvLog* Inst) override;
		void Visit(SpvExp2* Inst) override;
		void Visit(SpvLog2* Inst) override;
		void Visit(SpvSqrt* Inst) override;
		void Visit(SpvInverseSqrt* Inst) override;
		void Visit(SpvDeterminant* Inst) override;
		void Visit(SpvUMin* Inst) override;
		void Visit(SpvSMin* Inst) override;
		void Visit(SpvUMax* Inst) override;
		void Visit(SpvSMax* Inst) override;
		void Visit(SpvFClamp* Inst) override;
		void Visit(SpvUClamp* Inst) override;
		void Visit(SpvSClamp* Inst) override;
		void Visit(SpvFMix* Inst) override;
		void Visit(SpvStep* Inst) override;
		void Visit(SpvSmoothStep* Inst) override;
		void Visit(SpvPackHalf2x16* Inst) override;
		void Visit(SpvUnpackHalf2x16* Inst) override;
		void Visit(SpvLength* Inst) override;
		void Visit(SpvDistance* Inst) override;
		void Visit(SpvCross* Inst) override;
		void Visit(SpvNormalize* Inst) override;
		void Visit(SpvReflect* Inst) override;
		void Visit(SpvRefract* Inst) override;
		void Visit(SpvNMin* Inst) override;
		void Visit(SpvNMax* Inst) override;
		
		void Visit(SpvOpFunctionParameter* Inst) override;
		void Visit(SpvOpFunctionCall* Inst) override;
		void Visit(SpvOpVariable* Inst) override;
		void Visit(SpvOpPhi* Inst) override;
		void Visit(SpvOpLabel* Inst) override;
		void Visit(SpvOpLoad* Inst) override;
		void Visit(SpvOpStore* Inst) override;
		void Visit(SpvOpVectorShuffle* Inst) override;
		void Visit(SpvOpCompositeConstruct* Inst) override;
		void Visit(SpvOpCompositeExtract* Inst) override;
		void Visit(SpvOpTranspose* Inst) override;
		void Visit(SpvOpAccessChain* Inst) override;
		void Visit(SpvOpVectorTimesScalar* Inst) override;
		void Visit(SpvOpMatrixTimesScalar* Inst) override;
		void Visit(SpvOpVectorTimesMatrix* Inst) override;
		void Visit(SpvOpMatrixTimesVector* Inst) override;
		void Visit(SpvOpMatrixTimesMatrix* Inst) override;
		void Visit(SpvOpDot* Inst) override;
		void Visit(SpvOpAny* Inst) override;
		void Visit(SpvOpAll* Inst) override;
		void Visit(SpvOpIsNan* Inst) override;
		void Visit(SpvOpIsInf* Inst) override;
		void Visit(SpvOpLogicalOr* Inst) override;
		void Visit(SpvOpLogicalAnd* Inst) override;
		void Visit(SpvOpLogicalNot* Inst) override;
		void Visit(SpvOpSelect* Inst) override;
		void Visit(SpvOpIEqual* Inst) override;
		void Visit(SpvOpINotEqual* Inst) override;
		void Visit(SpvOpUGreaterThan* Inst) override;
		void Visit(SpvOpSGreaterThan* Inst) override;
		void Visit(SpvOpUGreaterThanEqual* Inst) override;
		void Visit(SpvOpSGreaterThanEqual* Inst) override;
		void Visit(SpvOpULessThan* Inst) override;
		void Visit(SpvOpSLessThan* Inst) override;
		void Visit(SpvOpULessThanEqual* Inst) override;
		void Visit(SpvOpSLessThanEqual* Inst) override;
		void Visit(SpvOpFOrdEqual* Inst) override;
		void Visit(SpvOpFOrdNotEqual* Inst) override;
		void Visit(SpvOpFOrdLessThan* Inst) override;
		void Visit(SpvOpFOrdGreaterThan* Inst) override;
		void Visit(SpvOpFOrdLessThanEqual* Inst) override;
		void Visit(SpvOpFOrdGreaterThanEqual* Inst) override;
		void Visit(SpvOpShiftRightLogical* Inst) override;
		void Visit(SpvOpShiftRightArithmetic* Inst) override;
		void Visit(SpvOpShiftLeftLogical* Inst) override;
		void Visit(SpvOpConvertFToU* Inst) override;
		void Visit(SpvOpConvertFToS* Inst) override;
		void Visit(SpvOpConvertSToF* Inst) override;
		void Visit(SpvOpConvertUToF* Inst) override;
		void Visit(SpvOpBitcast* Inst) override;
		void Visit(SpvOpSNegate* Inst) override;
		void Visit(SpvOpFNegate* Inst) override;
		void Visit(SpvOpIAdd* Inst) override;
		void Visit(SpvOpFAdd* Inst) override;
		void Visit(SpvOpISub* Inst) override;
		void Visit(SpvOpFSub* Inst) override;
		void Visit(SpvOpIMul* Inst) override;
		void Visit(SpvOpFMul* Inst) override;
		void Visit(SpvOpUDiv* Inst) override;
		void Visit(SpvOpSDiv* Inst) override;
		void Visit(SpvOpFDiv* Inst) override;
		void Visit(SpvOpUMod* Inst) override;
		void Visit(SpvOpSRem* Inst) override;
		void Visit(SpvOpFRem* Inst) override;
		void Visit(SpvOpBitwiseOr* Inst) override;
		void Visit(SpvOpBitwiseXor* Inst) override;
		void Visit(SpvOpBitwiseAnd* Inst) override;
		void Visit(SpvOpNot* Inst) override;
		void Visit(SpvOpBranch* Inst) override;
		void Visit(SpvOpBranchConditional* Inst) override;
		void Visit(SpvOpSwitch* Inst) override;
		void Visit(SpvOpReturn* Inst) override;
		void Visit(SpvOpReturnValue* Inst) override;
		void Visit(SpvOpSampledImage* Inst) override;
		void Visit(SpvOpImageFetch* Inst) override;
		void Visit(SpvOpImageQuerySizeLod* Inst) override;
		void Visit(SpvOpImageQueryLevels* Inst) override;

	protected:
		//if there is an ub case, the debugger results may not match gpu
		bool EnableUbsan = true;
		
		bool bTerminate{};
		const TArray<TUniquePtr<SpvInstruction>>* Insts;
	};

	SpvObject* GetObject(SpvVmContext* InContext, SpvId ObjectId);
	TArray<uint8> GetPointerValue(SpvVmContext* InContext, SpvPointer* InPointer);
	TArray<uint8> GetObjectValue(SpvObject* InObject, const TArray<uint32>& Indexes = {}, int32* OutOffset = nullptr);
	void WritePointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc, const TArray<uint8>& ValueToStore, SpvVariableChange* OutVariableChange = nullptr);
}
