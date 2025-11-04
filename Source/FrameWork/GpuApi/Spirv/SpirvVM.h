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
		const SpvPatcher& GetPatcher() const { return Patcher; }
		void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections) override;
		int32 GetInstIndex(SpvId Inst) const;
		SpvPointer* GetPointer(SpvId PointerId);
		TArray<uint8> ExecuteGpuOp(const FString& Name, int32 ResultSize, const TArray<uint8>& InputData, const TArray<FString>& Args, const TArray<GpuResource*>& InResources = {});
		
	public:
		void Visit(const SpvDebugLine* Inst) override;
		void Visit(const SpvDebugScope* Inst) override;
		void Visit(const SpvDebugDeclare* Inst) override;
		void Visit(const SpvDebugValue* Inst) override;
		void Visit(const SpvDebugFunctionDefinition* Inst) override;
		
		void Visit(const SpvRoundEven* Inst) override;
		void Visit(const SpvTrunc* Inst) override;
		void Visit(const SpvFAbs* Inst) override;
		void Visit(const SpvSAbs* Inst) override;
		void Visit(const SpvFSign* Inst) override;
		void Visit(const SpvSSign* Inst) override;
		void Visit(const SpvFloor* Inst) override;
		void Visit(const SpvCeil* Inst) override;
		void Visit(const SpvFract* Inst) override;
		void Visit(const SpvSin* Inst) override;
		void Visit(const SpvCos* Inst) override;
		void Visit(const SpvTan* Inst) override;
		void Visit(const SpvAsin* Inst) override;
		void Visit(const SpvAcos* Inst) override;
		void Visit(const SpvAtan* Inst) override;
		void Visit(const SpvPow* Inst) override;
		void Visit(const SpvExp* Inst) override;
		void Visit(const SpvLog* Inst) override;
		void Visit(const SpvExp2* Inst) override;
		void Visit(const SpvLog2* Inst) override;
		void Visit(const SpvSqrt* Inst) override;
		void Visit(const SpvInverseSqrt* Inst) override;
		void Visit(const SpvDeterminant* Inst) override;
		void Visit(const SpvUMin* Inst) override;
		void Visit(const SpvSMin* Inst) override;
		void Visit(const SpvUMax* Inst) override;
		void Visit(const SpvSMax* Inst) override;
		void Visit(const SpvFClamp* Inst) override;
		void Visit(const SpvUClamp* Inst) override;
		void Visit(const SpvSClamp* Inst) override;
		void Visit(const SpvFMix* Inst) override;
		void Visit(const SpvStep* Inst) override;
		void Visit(const SpvSmoothStep* Inst) override;
		void Visit(const SpvPackHalf2x16* Inst) override;
		void Visit(const SpvUnpackHalf2x16* Inst) override;
		void Visit(const SpvLength* Inst) override;
		void Visit(const SpvDistance* Inst) override;
		void Visit(const SpvCross* Inst) override;
		void Visit(const SpvNormalize* Inst) override;
		void Visit(const SpvReflect* Inst) override;
		void Visit(const SpvRefract* Inst) override;
		void Visit(const SpvNMin* Inst) override;
		void Visit(const SpvNMax* Inst) override;
		
		void Visit(const SpvOpFunctionParameter* Inst) override;
		void Visit(const SpvOpFunctionCall* Inst) override;
		void Visit(const SpvOpVariable* Inst) override;
		void Visit(const SpvOpPhi* Inst) override;
		void Visit(const SpvOpLabel* Inst) override;
		void Visit(const SpvOpLoad* Inst) override;
		void Visit(const SpvOpStore* Inst) override;
		void Visit(const SpvOpVectorShuffle* Inst) override;
		void Visit(const SpvOpCompositeConstruct* Inst) override;
		void Visit(const SpvOpCompositeExtract* Inst) override;
		void Visit(const SpvOpTranspose* Inst) override;
		void Visit(const SpvOpAccessChain* Inst) override;
		void Visit(const SpvOpVectorTimesScalar* Inst) override;
		void Visit(const SpvOpMatrixTimesScalar* Inst) override;
		void Visit(const SpvOpVectorTimesMatrix* Inst) override;
		void Visit(const SpvOpMatrixTimesVector* Inst) override;
		void Visit(const SpvOpMatrixTimesMatrix* Inst) override;
		void Visit(const SpvOpDot* Inst) override;
		void Visit(const SpvOpAny* Inst) override;
		void Visit(const SpvOpAll* Inst) override;
		void Visit(const SpvOpIsNan* Inst) override;
		void Visit(const SpvOpIsInf* Inst) override;
		void Visit(const SpvOpLogicalOr* Inst) override;
		void Visit(const SpvOpLogicalAnd* Inst) override;
		void Visit(const SpvOpLogicalNot* Inst) override;
		void Visit(const SpvOpSelect* Inst) override;
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
		void Visit(const SpvOpShiftRightLogical* Inst) override;
		void Visit(const SpvOpShiftRightArithmetic* Inst) override;
		void Visit(const SpvOpShiftLeftLogical* Inst) override;
		void Visit(const SpvOpConvertFToU* Inst) override;
		void Visit(const SpvOpConvertFToS* Inst) override;
		void Visit(const SpvOpConvertSToF* Inst) override;
		void Visit(const SpvOpConvertUToF* Inst) override;
		void Visit(const SpvOpBitcast* Inst) override;
		void Visit(const SpvOpSNegate* Inst) override;
		void Visit(const SpvOpFNegate* Inst) override;
		void Visit(const SpvOpIAdd* Inst) override;
		void Visit(const SpvOpFAdd* Inst) override;
		void Visit(const SpvOpISub* Inst) override;
		void Visit(const SpvOpFSub* Inst) override;
		void Visit(const SpvOpIMul* Inst) override;
		void Visit(const SpvOpFMul* Inst) override;
		void Visit(const SpvOpUDiv* Inst) override;
		void Visit(const SpvOpSDiv* Inst) override;
		void Visit(const SpvOpFDiv* Inst) override;
		void Visit(const SpvOpUMod* Inst) override;
		void Visit(const SpvOpSRem* Inst) override;
		void Visit(const SpvOpFRem* Inst) override;
		void Visit(const SpvOpBitwiseOr* Inst) override;
		void Visit(const SpvOpBitwiseXor* Inst) override;
		void Visit(const SpvOpBitwiseAnd* Inst) override;
		void Visit(const SpvOpNot* Inst) override;
		void Visit(const SpvOpBranch* Inst) override;
		void Visit(const SpvOpBranchConditional* Inst) override;
		void Visit(const SpvOpSwitch* Inst) override;
		void Visit(const SpvOpReturn* Inst) override;
		void Visit(const SpvOpReturnValue* Inst) override;
		void Visit(const SpvOpSampledImage* Inst) override;
		void Visit(const SpvOpImageFetch* Inst) override;
		void Visit(const SpvOpImageQuerySizeLod* Inst) override;
		void Visit(const SpvOpImageQueryLevels* Inst) override;

	protected:
		//if there is an ub case, the debugger results may not match gpu
		bool EnableUbsan = true;
		
		bool bTerminate{};
		const TArray<TUniquePtr<SpvInstruction>>* Insts;
		SpvId DebuggerBuffer;
		SpvPatcher Patcher;
	};

	SpvObject* GetObject(SpvVmContext* InContext, SpvId ObjectId);
	TArray<uint8> GetPointerValue(SpvVmContext* InContext, SpvPointer* InPointer);
	TArray<uint8> GetObjectValue(SpvObject* InObject, const TArray<uint32>& Indexes = {}, int32* OutOffset = nullptr);
	void WritePointerValue(SpvPointer* InPointer, SpvVariableDesc* PointeeDesc, const TArray<uint8>& ValueToStore, SpvVariableChange* OutVariableChange = nullptr);
}
