#pragma once
#include "SpirvCore.h"
#include "SpirvExt.h"

namespace FW
{
	class SpvInstruction;
	class FRAMEWORK_API SpvVisitor
	{
	public:
		virtual ~SpvVisitor() = default;
		virtual void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts, const TArray<uint32>& SpvCode, const TMap<SpvSectionKind, SpvSection>& InSections, const TMap<SpvId, SpvExtSet>& InExtSets) {}
		virtual void Visit(const SpvInstruction*) {}
		
		//Core
		virtual void Visit(const class SpvOpEntryPoint* Inst) {}
		virtual void Visit(const class SpvOpExecutionMode* Inst) {}
		virtual void Visit(const class SpvOpName* Inst) {}
		virtual void Visit(const class SpvOpString* Inst) {}
		virtual void Visit(const class SpvOpDecorate* Inst) {}
		virtual void Visit(const class SpvOpMemberDecorate* Inst) {}
		virtual void Visit(const class SpvOpTypeVoid* Inst) {}
		virtual void Visit(const class SpvOpTypeBool* Inst) {}
		virtual void Visit(const class SpvOpTypeFloat* Inst) {}
		virtual void Visit(const class SpvOpTypeInt* Inst) {}
		virtual void Visit(const class SpvOpTypeVector* Inst) {}
		virtual void Visit(const class SpvOpTypeMatrix* Inst) {}
		virtual void Visit(const class SpvOpTypePointer* Inst) {}
		virtual void Visit(const class SpvOpTypeFunction* Inst) {}
		virtual void Visit(const class SpvOpTypeStruct* Inst) {}
		virtual void Visit(const class SpvOpTypeImage* Inst) {}
		virtual void Visit(const class SpvOpTypeSampler* Inst) {}
		virtual void Visit(const class SpvOpTypeArray* Inst) {}
		virtual void Visit(const class SpvOpTypeRuntimeArray* Inst) {}
		
		virtual void Visit(const class SpvOpConstant* Inst) {}
		virtual void Visit(const class SpvOpConstantTrue* Inst) {}
		virtual void Visit(const class SpvOpConstantFalse* Inst) {}
		virtual void Visit(const class SpvOpConstantComposite* Inst) {}
		virtual void Visit(const class SpvOpConstantNull* Inst) {}
		virtual void Visit(const class SpvOpFunction* Inst) {}
		virtual void Visit(const class SpvOpFunctionParameter* Inst) {}
		virtual void Visit(const class SpvOpFunctionCall* Inst) {}
		virtual void Visit(const class SpvOpVariable* Inst) {}
		virtual void Visit(const class SpvOpAtomicIAdd* Inst) {}
		virtual void Visit(const class SpvOpPhi* Inst) {}
		virtual void Visit(const class SpvOpLoopMerge* Inst) {}
		virtual void Visit(const class SpvOpSelectionMerge* Inst) {}
		virtual void Visit(const class SpvOpLabel* Inst) {}
		virtual void Visit(const class SpvOpLoad* Inst) {}
		virtual void Visit(const class SpvOpStore* Inst) {}
		virtual void Visit(const class SpvOpVectorShuffle* Inst) {}
		virtual void Visit(const class SpvOpCompositeConstruct* Inst) {}
		virtual void Visit(const class SpvOpCompositeExtract* Inst) {}
		virtual void Visit(const class SpvOpTranspose* Inst) {}
		virtual void Visit(const class SpvOpAccessChain* Inst) {}
		virtual void Visit(const class SpvOpSampledImage* Inst) {}
		virtual void Visit(const class SpvOpImageSampleImplicitLod* Inst) {}
		virtual void Visit(const class SpvOpImageFetch* Inst) {}
		virtual void Visit(const class SpvOpImageQuerySizeLod* Inst) {}
		virtual void Visit(const class SpvOpImageQueryLevels* Inst) {}
		virtual void Visit(const class SpvOpVectorTimesScalar* Inst) {}
		virtual void Visit(const class SpvOpMatrixTimesScalar* Inst) {}
		virtual void Visit(const class SpvOpVectorTimesMatrix* Inst) {}
		virtual void Visit(const class SpvOpMatrixTimesVector* Inst) {}
		virtual void Visit(const class SpvOpMatrixTimesMatrix* Inst) {}
		virtual void Visit(const class SpvOpDot* Inst) {}
		virtual void Visit(const class SpvOpAny* Inst) {}
		virtual void Visit(const class SpvOpAll* Inst) {}
		virtual void Visit(const class SpvOpIsNan* Inst) {}
		virtual void Visit(const class SpvOpIsInf* Inst) {}
		virtual void Visit(const class SpvOpLogicalOr* Inst) {}
		virtual void Visit(const class SpvOpLogicalAnd* Inst) {}
		virtual void Visit(const class SpvOpLogicalNot* Inst) {}
		virtual void Visit(const class SpvOpSelect* Inst) {}
		virtual void Visit(const class SpvOpIEqual* Inst) {}
		virtual void Visit(const class SpvOpINotEqual* Inst) {}
		virtual void Visit(const class SpvOpUGreaterThan* Inst) {}
		virtual void Visit(const class SpvOpSGreaterThan* Inst) {}
		virtual void Visit(const class SpvOpUGreaterThanEqual* Inst) {}
		virtual void Visit(const class SpvOpSGreaterThanEqual* Inst) {}
		virtual void Visit(const class SpvOpULessThan* Inst) {}
		virtual void Visit(const class SpvOpSLessThan* Inst) {}
		virtual void Visit(const class SpvOpULessThanEqual* Inst) {}
		virtual void Visit(const class SpvOpSLessThanEqual* Inst) {}
		virtual void Visit(const class SpvOpFOrdEqual* Inst) {}
		virtual void Visit(const class SpvOpFOrdNotEqual* Inst) {}
		virtual void Visit(const class SpvOpFOrdLessThan* Inst) {}
		virtual void Visit(const class SpvOpFOrdGreaterThan* Inst) {}
		virtual void Visit(const class SpvOpFOrdLessThanEqual* Inst) {}
		virtual void Visit(const class SpvOpFOrdGreaterThanEqual* Inst) {}
		virtual void Visit(const class SpvOpShiftRightLogical* Inst) {}
		virtual void Visit(const class SpvOpShiftRightArithmetic* Inst) {}
		virtual void Visit(const class SpvOpShiftLeftLogical* Inst) {}
		virtual void Visit(const class SpvOpBitwiseOr* Inst) {}
		virtual void Visit(const class SpvOpBitwiseXor* Inst) {}
		virtual void Visit(const class SpvOpBitwiseAnd* Inst) {}
		virtual void Visit(const class SpvOpConvertFToU* Inst) {}
		virtual void Visit(const class SpvOpConvertFToS* Inst) {}
		virtual void Visit(const class SpvOpConvertSToF* Inst) {}
		virtual void Visit(const class SpvOpConvertUToF* Inst) {}
		virtual void Visit(const class SpvOpBitcast* Inst) {}
		virtual void Visit(const class SpvOpSNegate* Inst) {}
		virtual void Visit(const class SpvOpFNegate* Inst) {}
		virtual void Visit(const class SpvOpIAdd* Inst) {}
		virtual void Visit(const class SpvOpFAdd* Inst) {}
		virtual void Visit(const class SpvOpISub* Inst) {}
		virtual void Visit(const class SpvOpFSub* Inst) {}
		virtual void Visit(const class SpvOpIMul* Inst) {}
		virtual void Visit(const class SpvOpFMul* Inst) {}
		virtual void Visit(const class SpvOpUDiv* Inst) {}
		virtual void Visit(const class SpvOpSDiv* Inst) {}
		virtual void Visit(const class SpvOpFDiv* Inst) {}
		virtual void Visit(const class SpvOpUMod* Inst) {}
		virtual void Visit(const class SpvOpSRem* Inst) {}
		virtual void Visit(const class SpvOpFRem* Inst) {}
		virtual void Visit(const class SpvOpNot* Inst) {}
		virtual void Visit(const class SpvOpDPdx* Inst) {}
		virtual void Visit(const class SpvOpDPdy* Inst) {}
		virtual void Visit(const class SpvOpFwidth* Inst) {}
		virtual void Visit(const class SpvOpBranch* Inst) {}
		virtual void Visit(const class SpvOpBranchConditional* Inst) {}
		virtual void Visit(const class SpvOpSwitch* Inst) {}
		virtual void Visit(const class SpvOpKill* Inst) {}
		virtual void Visit(const class SpvOpReturn* Inst) {}
		virtual void Visit(const class SpvOpReturnValue* Inst) {}
		
		//NonSemantic.Shader.DebugInfo.100
		virtual void Visit(const class SpvDebugTypeBasic* Inst) {}
		virtual void Visit(const class SpvDebugTypeVector* Inst) {}
		virtual void Visit(const class SpvDebugTypeMatrix* Inst) {}
		virtual void Visit(const class SpvDebugTypeComposite* Inst) {}
		virtual void Visit(const class SpvDebugTypeMember* Inst) {}
		virtual void Visit(const class SpvDebugTypeArray* Inst) {}
		virtual void Visit(const class SpvDebugTypeTemplate* Inst) {}
		virtual void Visit(const class SpvDebugTypeFunction* Inst) {}
		virtual void Visit(const class SpvDebugCompilationUnit* Inst) {}
		virtual void Visit(const class SpvDebugLexicalBlock* Inst) {}
		virtual void Visit(const class SpvDebugFunction* Inst) {}
		virtual void Visit(const class SpvDebugSource* Inst) {}
		virtual void Visit(const class SpvDebugLine* Inst) {}
		virtual void Visit(const class SpvDebugScope* Inst) {}
		virtual void Visit(const class SpvDebugDeclare* Inst) {}
		virtual void Visit(const class SpvDebugValue* Inst) {}
		virtual void Visit(const class SpvDebugInlinedAt* Inst) {}
		virtual void Visit(const class SpvDebugLocalVariable* Inst) {}
		virtual void Visit(const class SpvDebugFunctionDefinition* Inst) {}
		virtual void Visit(const class SpvDebugGlobalVariable* Inst) {}
		
		//GLSL.std.450
		virtual void Visit(const class SpvRoundEven* Inst) {}
		virtual void Visit(const class SpvTrunc* Inst) {}
		virtual void Visit(const class SpvFAbs* Inst) {}
		virtual void Visit(const class SpvSAbs* Inst) {}
		virtual void Visit(const class SpvFSign* Inst) {}
		virtual void Visit(const class SpvSSign* Inst) {}
		virtual void Visit(const class SpvFloor* Inst) {}
		virtual void Visit(const class SpvCeil* Inst) {}
		virtual void Visit(const class SpvFract* Inst) {}
		virtual void Visit(const class SpvSin* Inst) {}
		virtual void Visit(const class SpvCos* Inst) {}
		virtual void Visit(const class SpvTan* Inst) {}
		virtual void Visit(const class SpvAsin* Inst) {}
		virtual void Visit(const class SpvAcos* Inst) {}
		virtual void Visit(const class SpvAtan* Inst) {}
		virtual void Visit(const class SpvAtan2* Inst) {}
		virtual void Visit(const class SpvPow* Inst) {}
		virtual void Visit(const class SpvExp* Inst) {}
		virtual void Visit(const class SpvLog* Inst) {}
		virtual void Visit(const class SpvExp2* Inst) {}
		virtual void Visit(const class SpvLog2* Inst) {}
		virtual void Visit(const class SpvSqrt* Inst) {}
		virtual void Visit(const class SpvInverseSqrt* Inst) {}
		virtual void Visit(const class SpvDeterminant* Isnt) {}
		virtual void Visit(const class SpvUMin* Inst) {}
		virtual void Visit(const class SpvSMin* Inst) {}
		virtual void Visit(const class SpvUMax* Inst) {}
		virtual void Visit(const class SpvSMax* Inst) {}
		virtual void Visit(const class SpvFClamp* Inst) {}
		virtual void Visit(const class SpvUClamp* Inst) {}
		virtual void Visit(const class SpvSClamp* Inst) {}
		virtual void Visit(const class SpvFMix* Inst) {}
		virtual void Visit(const class SpvStep* Inst) {}
		virtual void Visit(const class SpvSmoothStep* Inst) {}
		virtual void Visit(const class SpvPackHalf2x16* Inst) {}
		virtual void Visit(const class SpvUnpackHalf2x16* Inst) {}
		virtual void Visit(const class SpvLength* Inst) {}
		virtual void Visit(const class SpvDistance* Inst) {}
		virtual void Visit(const class SpvCross* Inst) {}
		virtual void Visit(const class SpvNormalize* Inst) {}
		virtual void Visit(const class SpvReflect* Inst) {}
		virtual void Visit(const class SpvRefract* Inst) {}
		virtual void Visit(const class SpvNMin* Inst) {}
		virtual void Visit(const class SpvNMax* Inst) {}
	};

	using SpvInstKind = std::variant<SpvOp, SpvGLSLstd450, SpvDebugInfo100>;

	class SpvInstruction
	{
	public:
		SpvInstruction(SpvInstKind InKind) : Kind(InKind) {}
		virtual ~SpvInstruction() = default;
		
	public:
		const SpvInstKind& GetKind() const { return Kind;}
		std::optional<SpvId> GetId() const { return ResultId; }
		void SetId(SpvId Id) { ResultId = Id; }
		std::optional<int> GetWordOffset() const { return WordOffset; }
		void SetWordOffset(uint32 InWordOffset) { WordOffset = InWordOffset; };
		std::optional<int> GetWordLen() const { return WordLen; }
		void SetWordLen(uint32 InWordLen) { WordLen = InWordLen; };

		virtual TUniquePtr<SpvInstruction> Clone() const = 0;
		virtual void Accept(SpvVisitor* Visitor) const = 0;
		virtual TArray<uint32> ToBinary() const { check(false);  return {}; }
		
	private:
		SpvInstKind Kind;
		std::optional<SpvId> ResultId;
		std::optional<int> WordOffset;
		std::optional<int> WordLen;
	};

	template<typename T>
	class SpvInstructionBase : public SpvInstruction
	{
	public:
		using SpvInstruction::SpvInstruction;

		TUniquePtr<SpvInstruction> Clone() const override
		{
			return MakeUnique<T>(static_cast<const T&>(*this));
		}
		
		void Accept(SpvVisitor* Visitor) const override
		{
			Visitor->Visit(static_cast<const T*>(this));
		}
	};

	class SpvOpExecutionMode : public SpvInstructionBase<SpvOpExecutionMode>
	{
	public:
		SpvOpExecutionMode(SpvId InEntryPoint, SpvExecutionMode InMode,
						   const TArray<uint8>& InOperands)
			: SpvInstructionBase(SpvOp::ExecutionMode)
			, EntryPoint(InEntryPoint)
			, Mode(InMode)
			, Operands(InOperands)
		{}
	public:
		SpvId GetEntryPointId() const { return EntryPoint; }
		SpvExecutionMode GetMode() const { return Mode; }
		const TArray<uint8>& GetExtraOperands() const { return Operands; }
		
	private:
		SpvId EntryPoint;
		SpvExecutionMode Mode;
		TArray<uint8> Operands;
	};

	class SpvOpEntryPoint : public SpvInstructionBase<SpvOpEntryPoint>
	{
	public:
		SpvOpEntryPoint(SpvExecutionModel InModel, SpvId InEntryPoint, const FString& InEntryPointName)
		: SpvInstructionBase(SpvOp::EntryPoint)
		, Model(InModel)
		, EntryPoint(InEntryPoint)
		, EntryPointName(InEntryPointName)
		{}
		
	private:
		SpvExecutionModel Model;
		SpvId EntryPoint;
		FString EntryPointName;
	};

	class SpvOpDecorate : public SpvInstructionBase<SpvOpDecorate>
	{
	public:
		SpvOpDecorate(SpvId InTarget, SpvDecorationKind InDecorationKind, const TArray<uint8>& InOperands = {})
		: SpvInstructionBase(SpvOp::Decorate)
		, Target(InTarget)
		, DecorationKind(InDecorationKind)
		, Operands(InOperands)
		{}
		
		SpvId GetTargetId() const { return Target; }
		SpvDecorationKind GetKind() const { return DecorationKind; }
		const TArray<uint8>& GetExtraOperands() const { return Operands; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(Target.GetValue());
			Bin.Add((uint32)DecorationKind);
			Bin.Append((uint32*)Operands.GetData(), Operands.Num() / 4);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Decorate;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId Target;
		SpvDecorationKind DecorationKind;
		TArray<uint8> Operands;
	};

	class SpvOpMemberDecorate : public SpvInstructionBase<SpvOpMemberDecorate>
	{
	public:
		SpvOpMemberDecorate(SpvId InStructureType, uint32 InMember, SpvDecorationKind InDecorationKind, const TArray<uint8>& InOperands)
		: SpvInstructionBase(SpvOp::MemberDecorate)
		, StructureType(InStructureType)
		, Member(InMember)
		, DecorationKind(InDecorationKind)
		, Operands(InOperands)
		{}
		
		SpvId GetStructureType() const { return StructureType; }
		uint32 GetMember() const { return Member; }
		SpvDecorationKind GetKind() const { return DecorationKind; }
		const TArray<uint8>& GetExtraOperands() const { return Operands; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(StructureType.GetValue());
			Bin.Add(Member);
			Bin.Add((uint32)DecorationKind);
			Bin.Append((uint32*)Operands.GetData(), Operands.Num() / 4);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::MemberDecorate;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId StructureType;
		uint32 Member;
		SpvDecorationKind DecorationKind;
		TArray<uint8> Operands;
	};

	class SpvOpTypeFloat : public SpvInstructionBase<SpvOpTypeFloat>
	{
	public:
		SpvOpTypeFloat(uint32 InWidth)
		: SpvInstructionBase(SpvOp::TypeFloat)
		, Width(InWidth)
		{}
		uint32 GetWidth() const { return Width; }
		
	private:
		uint32 Width;
	};

	class SpvOpTypeInt : public SpvInstructionBase<SpvOpTypeInt>
	{
	public:
		SpvOpTypeInt(uint32 InWidth, uint32 InSignedness)
		: SpvInstructionBase(SpvOp::TypeInt)
		, Width(InWidth)
		, Signedness(InSignedness)
		{}
		
		uint32 GetWidth() const { return Width; }
		uint32 GetSignedness() const { return Signedness; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Width);
			Bin.Add(Signedness);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::TypeInt;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		uint32 Width;
		uint32 Signedness;
	};

	class SpvOpTypeVector: public SpvInstructionBase<SpvOpTypeVector>
	{
	public:
		SpvOpTypeVector(SpvId InComponentType, uint32 InComponentCount)
		: SpvInstructionBase(SpvOp::TypeVector)
		, ComponentType(InComponentType)
		, ComponentCount(InComponentCount)
		{}
		
		SpvId GetComponentType() const { return ComponentType; }
		uint32 GetComponentCount() const { return ComponentCount; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(GetId().value().GetValue());
			Bin.Add(ComponentType.GetValue());
			Bin.Add(ComponentCount);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::TypeVector;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ComponentType;
		uint32 ComponentCount;
	};

	class SpvOpTypeMatrix: public SpvInstructionBase<SpvOpTypeMatrix>
	{
	public:
		SpvOpTypeMatrix(SpvId InColumnType, uint32 InColumnCount)
		: SpvInstructionBase(SpvOp::TypeMatrix)
		, ColumnType(InColumnType)
		, ColumnCount(InColumnCount)
		{}
		
		SpvId GetColumnType() const { return ColumnType; }
		uint32 GetColumnCount() const { return ColumnCount; }
		
	private:
		SpvId ColumnType;
		uint32 ColumnCount;
	};

	class SpvOpTypeVoid : public SpvInstructionBase<SpvOpTypeVoid>
	{
	public:
		SpvOpTypeVoid() : SpvInstructionBase(SpvOp::TypeVoid) {}
	};

	class SpvOpTypeBool : public SpvInstructionBase<SpvOpTypeBool>
	{
	public:
		SpvOpTypeBool() : SpvInstructionBase(SpvOp::TypeBool) {}
	};

	class SpvOpTypePointer : public SpvInstructionBase<SpvOpTypePointer>
	{
	public:
		SpvOpTypePointer(SpvStorageClass InStorageClass, SpvId InType)
		: SpvInstructionBase(SpvOp::TypePointer)
		, StorageClass(InStorageClass)
		, PointeeType(InType)
		{}
		
		SpvStorageClass GetStorageClass() const { return StorageClass; }
		SpvId GetPointeeType() const { return PointeeType; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(GetId().value().GetValue());
			Bin.Add((uint32)StorageClass);
			Bin.Add(PointeeType.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::TypePointer;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvStorageClass StorageClass;
		SpvId PointeeType;
	};

	class SpvOpTypeFunction : public SpvInstructionBase<SpvOpTypeFunction>
	{
	public:
		SpvOpTypeFunction(SpvId InReturnType, const TArray<SpvId>& InParameterTypes = {}) : SpvInstructionBase(SpvOp::TypeFunction)
		, ReturnType(InReturnType)
		, ParameterTypes(InParameterTypes)
		{}

		SpvId GetReturnType() const { return ReturnType; }
		const TArray<SpvId>& GetParameterTypes() const { return ParameterTypes; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(GetId().value().GetValue());
			Bin.Add(ReturnType.GetValue());
			Bin.Append((uint32*)ParameterTypes.GetData(), ParameterTypes.Num());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::TypeFunction;
			Bin.Insert(Header, 0);
			return Bin;
		}

	private:
		SpvId ReturnType;
		TArray<SpvId> ParameterTypes;
	};

	class SpvOpTypeStruct : public SpvInstructionBase<SpvOpTypeStruct>
	{
	public:
		SpvOpTypeStruct(const TArray<SpvId>& InMemberTypes)
		: SpvInstructionBase(SpvOp::TypeStruct)
		, MemberTypes(InMemberTypes)
		{}
		
		const TArray<SpvId>& GetMemberTypeIds() const { return MemberTypes; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(GetId().value().GetValue());
			Bin.Append((uint32*)MemberTypes.GetData(), MemberTypes.Num());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::TypeStruct;
			Bin.Insert(Header, 0);
			return Bin;
		}
	
	private:
		TArray<SpvId> MemberTypes;
	};

	class SpvOpTypeImage : public SpvInstructionBase<SpvOpTypeImage>
	{
	public:
		SpvOpTypeImage(SpvId InSampledType, SpvDim InDim, uint32 InDepth, uint32 InArrayed, uint32 InMS, uint32 InSampled, SpvImageFormat InFormat)
		: SpvInstructionBase(SpvOp::TypeImage)
		, SampledType(InSampledType)
		, Dim(InDim)
		, Depth(InDepth), Arrayed(InArrayed), MS(InMS), Sampled(InSampled)
		, Format(InFormat)
		{}
		
		SpvId GetSampledType() const { return SampledType; }
		SpvDim GetDim() const { return Dim; }
		uint32 GetDepth() const { return Depth; }
		uint32 GetArrayed() const { return Arrayed; }
		uint32 GetMS() const { return MS; }
		uint32 GetSamlped() const { return Sampled; }
		SpvImageFormat GetFormat() const { return Format; }
		
	private:
		SpvId SampledType;
		SpvDim Dim;
		uint32 Depth, Arrayed, MS, Sampled;
		SpvImageFormat Format;
	};

	class SpvOpTypeSampler : public SpvInstructionBase<SpvOpTypeSampler>
	{
	public:
		SpvOpTypeSampler() : SpvInstructionBase(SpvOp::TypeSampler) {}
	};

	class SpvOpTypeArray : public SpvInstructionBase<SpvOpTypeArray>
	{
	public:
		SpvOpTypeArray(SpvId InElementType, SpvId InLength) : SpvInstructionBase(SpvOp::TypeArray)
		, ElementType(InElementType), Length(InLength)
		{}
		
		SpvId GetElementType() const { return ElementType; }
		SpvId GetLength() const { return Length; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(GetId().value().GetValue());
			Bin.Add(ElementType.GetValue());
			Bin.Add(Length.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::TypeArray;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ElementType;
		SpvId Length;
	};

	class SpvOpTypeRuntimeArray : public SpvInstructionBase<SpvOpTypeRuntimeArray>
	{
	public:
		SpvOpTypeRuntimeArray(SpvId InElementType) : SpvInstructionBase(SpvOp::TypeRuntimeArray)
		, ElementType(InElementType)
		{}
		
		SpvId GetElementType() const { return ElementType; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(GetId().value().GetValue());
			Bin.Add(ElementType.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::TypeRuntimeArray;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ElementType;
	};

	class SpvOpConstant : public SpvInstructionBase<SpvOpConstant>
	{
	public:
		SpvOpConstant(SpvId InResultType, const TArray<uint8>& InValue, SpvOp OpCode = SpvOp::Constant)
		: SpvInstructionBase(OpCode)
		, ResultType(InResultType)
		, Value(InValue)
		{}
		SpvId GetResultType() const { return ResultType; }
		const TArray<uint8>& GetValue() const { return Value; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Append((uint32*)Value.GetData(), Value.Num() / 4);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Constant;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	protected:
		SpvId ResultType;
		TArray<uint8> Value;
	};

	class SpvOpConstantTrue : public SpvOpConstant
	{
	public:
		SpvOpConstantTrue(SpvId InResultType)
		: SpvOpConstant(InResultType, {(uint8*)new uint32{1}, 4}, SpvOp::ConstantTrue)
		{}
	};

	class SpvOpConstantFalse : public SpvOpConstant
	{
	public:
		SpvOpConstantFalse(SpvId InResultType)
		: SpvOpConstant(InResultType, {(uint8*)new uint32{0}, 4}, SpvOp::ConstantFalse)
		{}
	};

	class SpvOpConstantComposite : public SpvInstructionBase<SpvOpConstantComposite>
	{
	public:
		SpvOpConstantComposite(SpvId InResultType, const TArray<SpvId>& InConstituents)
		: SpvInstructionBase(SpvOp::ConstantComposite)
		, ResultType(InResultType)
		, Constituents(InConstituents)
		{}
		SpvId GetResultType() const { return ResultType; }
		const TArray<SpvId>& GetConstituents() const { return Constituents; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Append((uint32*)Constituents.GetData(), Constituents.Num());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::ConstantComposite;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		TArray<SpvId> Constituents;
	};

	class SpvOpConstantNull : public SpvInstructionBase<SpvOpConstantNull>
	{
	public:
		SpvOpConstantNull(SpvId InResultType) : SpvInstructionBase(SpvOp::ConstantNull)
		, ResultType(InResultType)
		{}
		SpvId GetResultType() const { return ResultType; }
	private:
		SpvId ResultType;
	};

	class SpvOpName : public SpvInstructionBase<SpvOpName>
	{
	public:
		SpvOpName(SpvId InTarget, const FString& InName)
		: SpvInstructionBase(SpvOp::Name)
		, Target(InTarget)
		, Name(InName)
		{}
		
		SpvId GetTarget() const { return Target; }
		const FString& GetName() const { return Name; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(Target.GetValue());
			std::string Utf8Name(TCHAR_TO_UTF8(*Name));
			Bin.Append((uint32*)Utf8Name.c_str(), ((int)Utf8Name.size() + 1 + 3) / 4);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Name;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId Target;
		FString Name;
	};

	class SpvOpString : public SpvInstructionBase<SpvOpString>
	{
	public:
		SpvOpString(const FString& InStr)
		: SpvInstructionBase(SpvOp::String)
		, Str(InStr)
		{}
		
		const FString& GetStr() const { return Str; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(GetId().value().GetValue());
			std::string Utf8Name(TCHAR_TO_UTF8(*Str));
			Bin.Append((uint32*)Utf8Name.c_str(), ((int)Utf8Name.size() + 1 + 3) / 4);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::String;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		FString Str;
	};

	class SpvOpFunction : public SpvInstructionBase<SpvOpFunction>
	{
	public:
		SpvOpFunction(SpvId InResultType, SpvFunctionControl InFunctionControl, SpvId InFunctionType) : SpvInstructionBase(SpvOp::Function)
		, ResultType(InResultType)
		, FunctionControl(InFunctionControl)
		, FunctionType(InFunctionType)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvFunctionControl GetFunctionControl() const { return FunctionControl; }
		SpvId GetFunctionType() const { return FunctionType; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add((uint32)FunctionControl);
			Bin.Add(FunctionType.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Function;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvFunctionControl FunctionControl;
		SpvId FunctionType;
	};

	class SpvOpFunctionParameter : public SpvInstructionBase<SpvOpFunctionParameter>
	{
	public:
		SpvOpFunctionParameter(SpvId InResultType) : SpvInstructionBase(SpvOp::FunctionParameter)
		, ResultType(InResultType)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::FunctionParameter;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
	};

	class SpvOpFunctionEnd : public SpvInstructionBase<SpvOpFunctionEnd>
	{
	public:
		SpvOpFunctionEnd() : SpvInstructionBase(SpvOp::FunctionEnd) {}
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::FunctionEnd;
			Bin.Insert(Header, 0);
			return Bin;
		}
	};

	class SpvOpFunctionCall : public SpvInstructionBase<SpvOpFunctionCall>
	{
	public:
		SpvOpFunctionCall(SpvId InResultType, SpvId InFunction, const TArray<SpvId>& InArguments = {}) : SpvInstructionBase(SpvOp::FunctionCall)
		, ResultType(InResultType)
		, Function(InFunction)
		, Arguments(InArguments)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetFunction() const { return Function; }
		const TArray<SpvId>& GetArguments() const { return Arguments; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Function.GetValue());
			Bin.Append((uint32*)Arguments.GetData(), Arguments.Num());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::FunctionCall;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Function;
		TArray<SpvId> Arguments;
	};

	class SpvOpVariable : public SpvInstructionBase<SpvOpVariable>
	{
	public:
		SpvOpVariable(SpvId InResultType, SpvStorageClass InStorageClass, std::optional<SpvId> InInitializer = {}) : SpvInstructionBase(SpvOp::Variable)
		, ResultType(InResultType)
		, StorageClass(InStorageClass)
		, Initializer(InInitializer)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvStorageClass GetStorageClass() const { return StorageClass; }
		std::optional<SpvId> GetInitializer() const { return Initializer; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add((uint32)StorageClass);
			if (Initializer)
			{
				Bin.Add(Initializer.value().GetValue());
			}
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Variable;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvStorageClass StorageClass;
		std::optional<SpvId> Initializer;
	};

	class SpvOpAtomicIAdd : public SpvInstructionBase<SpvOpAtomicIAdd>
	{
	public:
		SpvOpAtomicIAdd(SpvId InResultType, SpvId InPointer, SpvId InMemory, SpvId InSemantics, SpvId InValue)
			: SpvInstructionBase(SpvOp::AtomicIAdd)
			, ResultType(InResultType), Pointer(InPointer)
			, Memory(InMemory), Semantics(InSemantics), Value(InValue)
		{}

		SpvId GetResultType() const { return ResultType; }
		SpvId GetPointer() const { return Pointer; }
		SpvId GetMemory() const { return Memory; }
		SpvId GetSemantics() const { return Semantics; }
		SpvId GetValue() const { return Value; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Pointer.GetValue());
			Bin.Add(Memory.GetValue());
			Bin.Add(Semantics.GetValue());
			Bin.Add(Value.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::AtomicIAdd;
			Bin.Insert(Header, 0);
			return Bin;
		}

	private:
		SpvId ResultType;
		SpvId Pointer;
		SpvId Memory;
		SpvId Semantics;
		SpvId Value;
	};

	class SpvOpPhi : public SpvInstructionBase<SpvOpPhi>
	{
	public:
		SpvOpPhi(SpvId InResultType, const TArray<TPair<SpvId, SpvId>>& InOperands) : SpvInstructionBase(SpvOp::Phi)
		, ResultType(InResultType), Operands(InOperands)
		{}

		SpvId GetResultType() const { return ResultType; }
		const TArray<TPair<SpvId, SpvId>>& GetOperands() const { return Operands; }

	private:
		SpvId ResultType;
		TArray<TPair<SpvId, SpvId>> Operands;
	};

	class SpvOpLoopMerge : public SpvInstructionBase<SpvOpLoopMerge>
	{
	public:
		SpvOpLoopMerge(SpvId InMergeBlock, SpvId InContinueBlock, SpvLoopControl InLoopControl) : SpvInstructionBase(SpvOp::LoopMerge)
		, MergeBlock(InMergeBlock), ContinueBlock(InContinueBlock), LoopControl(InLoopControl)
		{}

		SpvId GetMergeBlock() const { return MergeBlock; }
		SpvId GetContinueBlock() const { return ContinueBlock; }
		SpvLoopControl GetLoopControl() const { return LoopControl; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(MergeBlock.GetValue());
			Bin.Add(ContinueBlock.GetValue());
			Bin.Add((uint32)LoopControl);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::LoopMerge;
			Bin.Insert(Header, 0);
			return Bin;
		}

	private:
		SpvId MergeBlock;
		SpvId ContinueBlock;
		SpvLoopControl LoopControl;
	};

	class SpvOpSelectionMerge : public SpvInstructionBase<SpvOpSelectionMerge>
	{
	public:
		SpvOpSelectionMerge(SpvId InMergeBlock, SpvSelectionControl InSelectionControl) : SpvInstructionBase(SpvOp::SelectionMerge)
		, MergeBlock(InMergeBlock), SelectionControl(InSelectionControl)
		{}

		SpvId GetMergeBlock() const { return MergeBlock; }
		SpvSelectionControl GetSelectionControl() const { return SelectionControl; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(MergeBlock.GetValue());
			Bin.Add((uint32)SelectionControl);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::SelectionMerge;
			Bin.Insert(Header, 0);
			return Bin;
		}

	private:
		SpvId MergeBlock;
		SpvSelectionControl SelectionControl;
	};

	class SpvOpLabel : public SpvInstructionBase<SpvOpLabel>
	{
	public:
		SpvOpLabel() : SpvInstructionBase(SpvOp::Label) {}
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(GetId().value().GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Label;
			Bin.Insert(Header, 0);
			return Bin;
		}
	};

	class SpvOpLoad : public SpvInstructionBase<SpvOpLoad>
	{
	public:
		SpvOpLoad(SpvId InResultType, SpvId InPointer) : SpvInstructionBase(SpvOp::Load)
		, ResultType(InResultType)
		, Pointer(InPointer)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetPointer() const { return Pointer; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Pointer.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Load;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Pointer;
	};

	class SpvOpStore : public SpvInstructionBase<SpvOpStore>
	{
	public:
		SpvOpStore(SpvId InPointer, SpvId InObject) : SpvInstructionBase(SpvOp::Store)
		, Pointer(InPointer)
		, Object(InObject)
		{}
		
		SpvId GetPointer() const { return Pointer; }
		SpvId GetObject() const { return Object; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(Pointer.GetValue());
			Bin.Add(Object.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Store;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId Pointer;
		SpvId Object;
	};

	class SpvOpVectorShuffle : public SpvInstructionBase<SpvOpVectorShuffle>
	{
	public:
		SpvOpVectorShuffle(SpvId InResultType, SpvId InVector1, SpvId InVector2, const TArray<uint32>& InComponents) : SpvInstructionBase(SpvOp::VectorShuffle)
		, ResultType(InResultType), Vector1(InVector1), Vector2(InVector2), Components(InComponents)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetVector1() const { return Vector1; }
		SpvId GetVector2() const { return Vector2; }
		const TArray<uint32>& GetComponents() const { return Components; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Vector1.GetValue());
			Bin.Add(Vector2.GetValue());
			Bin.Append(Components);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::VectorShuffle;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Vector1;
		SpvId Vector2;
		TArray<uint32> Components;
	};

	class SpvOpCompositeConstruct : public SpvInstructionBase<SpvOpCompositeConstruct>
	{
	public:
		SpvOpCompositeConstruct(SpvId InResultType, const TArray<SpvId>& InConstituents) : SpvInstructionBase(SpvOp::CompositeConstruct)
		, ResultType(InResultType)
		, Constituents(InConstituents)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		const TArray<SpvId>& GetConstituents() const { return Constituents; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Append((uint32*)Constituents.GetData(), Constituents.Num());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::CompositeConstruct;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		TArray<SpvId> Constituents;
	};

	class SpvOpCompositeExtract : public SpvInstructionBase<SpvOpCompositeExtract>
	{
	public:
		SpvOpCompositeExtract(SpvId InResultType, SpvId InComposite, const TArray<uint32>& InIndexes) : SpvInstructionBase(SpvOp::CompositeExtract)
		, ResultType(InResultType), Composite(InComposite), Indexes(InIndexes)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetComposite() const { return Composite; }
		const TArray<uint32>& GetIndexes() const { return Indexes; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Composite.GetValue());
			Bin.Append(Indexes);
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::CompositeExtract;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Composite;
		TArray<uint32> Indexes;
	};

	class SpvOpTranspose : public SpvInstructionBase<SpvOpTranspose>
	{
	public:
		SpvOpTranspose(SpvId InResultType, SpvId InMatrix) : SpvInstructionBase(SpvOp::Transpose)
		, ResultType(InResultType), Matrix(InMatrix)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetMatrix() const { return Matrix; }
		
	private:
		SpvId ResultType;
		SpvId Matrix;
	};

	class SpvOpAccessChain : public SpvInstructionBase<SpvOpAccessChain>
	{
	public:
		SpvOpAccessChain(SpvId InResultType, SpvId InBasePointer, const TArray<SpvId>& InIndexes) : SpvInstructionBase(SpvOp::AccessChain)
		, ResultType(InResultType)
		, BasePointer(InBasePointer)
		, Indexes(InIndexes)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetBasePointer() const { return BasePointer; }
		const TArray<SpvId>& GetIndexes() const { return Indexes; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(BasePointer.GetValue());
			Bin.Append((uint32*)Indexes.GetData(), Indexes.Num());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::AccessChain;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId BasePointer;
		TArray<SpvId> Indexes;
	};

	class SpvOpSampledImage : public SpvInstructionBase<SpvOpSampledImage>
	{
	public:
		SpvOpSampledImage(SpvId InResultType, SpvId InImage, SpvId InSampler) : SpvInstructionBase(SpvOp::SampledImage)
		, ResultType(InResultType), Image(InImage), Sampler(InSampler)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetImage() const { return Image; }
		SpvId GetSampler() const { return Sampler; }
		
	private:
		SpvId ResultType;
		SpvId Image;
		SpvId Sampler;
	};

	class SpvOpImageSampleImplicitLod : public SpvInstructionBase<SpvOpImageSampleImplicitLod>
	{
	public:
		SpvOpImageSampleImplicitLod(SpvId InResultType, SpvId InSampledImage, SpvId InCoordinate, std::optional<SpvImageOperands> InImageOperands, const TArray<SpvId>& InOperands) : SpvInstructionBase(SpvOp::ImageSampleImplicitLod)
		, ResultType(InResultType), SampledImage(InSampledImage), Coordinate(InCoordinate), ImageOperands(InImageOperands), Operands(InOperands)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetSampledImage() const { return SampledImage; }
		SpvId GetCoordinate() const { return Coordinate; }
		std::optional<SpvImageOperands> GetImageOperands() const { return ImageOperands; }
		const TArray<SpvId>& GetOperands() const { return Operands; }
		
	private:
		SpvId ResultType;
		SpvId SampledImage;
		SpvId Coordinate;
		std::optional<SpvImageOperands> ImageOperands;
		TArray<SpvId> Operands;
	};

	class SpvOpImageFetch : public SpvInstructionBase<SpvOpImageFetch>
	{
	public:
		SpvOpImageFetch(SpvId InResultType, SpvId InImage, SpvId InCoordinate, std::optional<SpvImageOperands> InImageOperands, const TArray<SpvId>& InOperands) : SpvInstructionBase(SpvOp::ImageFetch)
		, ResultType(InResultType), Image(InImage), Coordinate(InCoordinate), ImageOperands(InImageOperands), Operands(InOperands)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetImage() const { return Image; }
		SpvId GetCoordinate() const { return Coordinate; }
		std::optional<SpvImageOperands> GetImageOperands() const { return ImageOperands; }
		const TArray<SpvId>& GetOperands() const { return Operands; }
		
	private:
		SpvId ResultType;
		SpvId Image;
		SpvId Coordinate;
		std::optional<SpvImageOperands> ImageOperands;
		TArray<SpvId> Operands;
	};

	class SpvOpImageQuerySizeLod : public SpvInstructionBase<SpvOpImageQuerySizeLod>
	{
	public:
		SpvOpImageQuerySizeLod(SpvId InResultType, SpvId InImage, SpvId InLod) : SpvInstructionBase(SpvOp::ImageQuerySizeLod)
		, ResultType(InResultType), Image(InImage), Lod(InLod)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetImage() const { return Image; }
		SpvId GetLod() const { return Lod; }
		
	private:
		SpvId ResultType;
		SpvId Image;
		SpvId Lod;
	};

	class SpvOpImageQueryLevels : public SpvInstructionBase<SpvOpImageQueryLevels>
	{
	public:
		SpvOpImageQueryLevels(SpvId InResultType, SpvId InImage) : SpvInstructionBase(SpvOp::ImageQueryLevels)
		, ResultType(InResultType), Image(InImage)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetImage() const { return Image; }
		
	private:
		SpvId ResultType;
		SpvId Image;
	};

	class SpvOpConvertFToU : public SpvInstructionBase<SpvOpConvertFToU>
	{
	public:
		SpvOpConvertFToU(SpvId InResultType, SpvId InFloatValue) : SpvInstructionBase(SpvOp::ConvertFToU)
		, ResultType(InResultType), FloatValue(InFloatValue)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetFloatValue() const { return FloatValue; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(FloatValue.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::ConvertFToU;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId FloatValue;
	};

	class SpvOpConvertFToS : public SpvInstructionBase<SpvOpConvertFToS>
	{
	public:
		SpvOpConvertFToS(SpvId InResultType, SpvId InFloatValue) : SpvInstructionBase(SpvOp::ConvertFToS)
		, ResultType(InResultType), FloatValue(InFloatValue)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetFloatValue() const { return FloatValue; }
		
	private:
		SpvId ResultType;
		SpvId FloatValue;
	};

	class SpvOpConvertSToF : public SpvInstructionBase<SpvOpConvertSToF>
	{
	public:
		SpvOpConvertSToF(SpvId InResultType, SpvId InSignedValue) : SpvInstructionBase(SpvOp::ConvertSToF)
		, ResultType(InResultType), SignedValue(InSignedValue)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetSignedValue() const { return SignedValue; }
		
	private:
		SpvId ResultType;
		SpvId SignedValue;
	};

	class SpvOpConvertUToF : public SpvInstructionBase<SpvOpConvertUToF>
	   {
	   public:
		   SpvOpConvertUToF(SpvId InResultType, SpvId InUnsignedValue) : SpvInstructionBase(SpvOp::ConvertUToF)
		   , ResultType(InResultType), UnsignedValue(InUnsignedValue)
		   {}
		   
		   SpvId GetResultType() const { return ResultType; }
		   SpvId GetUnsignedValue() const { return UnsignedValue; }
		   
	   private:
		   SpvId ResultType;
		   SpvId UnsignedValue;
	   };

	class SpvOpBitcast : public SpvInstructionBase<SpvOpBitcast>
	{
	public:
		SpvOpBitcast(SpvId InResultType, SpvId InOperand) : SpvInstructionBase(SpvOp::Bitcast)
		, ResultType(InResultType), Operand(InOperand)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand() const { return Operand; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Operand.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Bitcast;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Operand;
	};

	class SpvOpSNegate : public SpvInstructionBase<SpvOpSNegate>
	{
	public:
		SpvOpSNegate(SpvId InResultType, SpvId InOperand) : SpvInstructionBase(SpvOp::SNegate)
		, ResultType(InResultType), Operand(InOperand)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand() const { return Operand; }
		
	private:
		SpvId ResultType;
		SpvId Operand;
	};

	class SpvOpFNegate : public SpvInstructionBase<SpvOpFNegate>
	{
	public:
		SpvOpFNegate(SpvId InResultType, SpvId InOperand) : SpvInstructionBase(SpvOp::FNegate)
		, ResultType(InResultType), Operand(InOperand)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand() const { return Operand; }
		
	private:
		SpvId ResultType;
		SpvId Operand;
	};

	class SpvOpIAdd : public SpvInstructionBase<SpvOpIAdd>
	{
	public:
		SpvOpIAdd(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::IAdd)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Operand1.GetValue());
			Bin.Add(Operand2.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::IAdd;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpFAdd : public SpvInstructionBase<SpvOpFAdd>
	{
	public:
		SpvOpFAdd(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::FAdd)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpISub : public SpvInstructionBase<SpvOpISub>
	{
	public:
		SpvOpISub(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::ISub)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpFSub : public SpvInstructionBase<SpvOpFSub>
	{
	public:
		SpvOpFSub(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::FSub)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpIMul : public SpvInstructionBase<SpvOpIMul>
	{
	public:
		SpvOpIMul(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::IMul)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpFMul : public SpvInstructionBase<SpvOpFMul>
	{
	public:
		SpvOpFMul(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::FMul)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpUDiv : public SpvInstructionBase<SpvOpUDiv>
	{
	public:
		SpvOpUDiv(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::UDiv)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpSDiv : public SpvInstructionBase<SpvOpSDiv>
	{
	public:
		SpvOpSDiv(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::SDiv)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpFDiv : public SpvInstructionBase<SpvOpFDiv>
	{
	public:
		SpvOpFDiv(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::FDiv)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpUMod : public SpvInstructionBase<SpvOpUMod>
	{
	public:
		SpvOpUMod(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::UMod)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpSRem : public SpvInstructionBase<SpvOpSRem>
	{
	public:
		SpvOpSRem(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::SRem)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpFRem : public SpvInstructionBase<SpvOpFRem>
	{
	public:
		SpvOpFRem(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::FRem)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpVectorTimesScalar : public SpvInstructionBase<SpvOpVectorTimesScalar>
	{
	public:
		SpvOpVectorTimesScalar(SpvId InResultType, SpvId InVector, SpvId InScalar) : SpvInstructionBase(SpvOp::VectorTimesScalar)
		, ResultType(InResultType), Vector(InVector), Scalar(InScalar)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetVector() const { return Vector; }
		SpvId GetScalar() const { return Scalar; }
		
	private:
		SpvId ResultType;
		SpvId Vector;
		SpvId Scalar;
	};

	class SpvOpMatrixTimesScalar : public SpvInstructionBase<SpvOpMatrixTimesScalar>
	{
	public:
		SpvOpMatrixTimesScalar(SpvId InResultType, SpvId InMatrix, SpvId InScalar) : SpvInstructionBase(SpvOp::MatrixTimesScalar)
		, ResultType(InResultType), Matrix(InMatrix), Scalar(InScalar)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetMatrix() const { return Matrix; }
		SpvId GetScalar() const { return Scalar; }
		
	private:
		SpvId ResultType;
		SpvId Matrix;
		SpvId Scalar;
	};

	class SpvOpVectorTimesMatrix : public SpvInstructionBase<SpvOpVectorTimesMatrix>
	{
	public:
		SpvOpVectorTimesMatrix(SpvId InResultType, SpvId InVector, SpvId InMatrix) : SpvInstructionBase(SpvOp::VectorTimesMatrix)
		, ResultType(InResultType), Vector(InVector), Matrix(InMatrix)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetVector() const { return Vector; }
		SpvId GetMatrix() const { return Matrix; }
		
	private:
		SpvId ResultType;
		SpvId Vector;
		SpvId Matrix;
	};

	class SpvOpMatrixTimesVector : public SpvInstructionBase<SpvOpMatrixTimesVector>
	{
	public:
		SpvOpMatrixTimesVector(SpvId InResultType, SpvId InMatrix, SpvId InVector) : SpvInstructionBase(SpvOp::MatrixTimesVector)
		, ResultType(InResultType), Matrix(InMatrix), Vector(InVector)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetMatrix() const { return Matrix; }
		SpvId GetVector() const { return Vector; }
		
	private:
		SpvId ResultType;
		SpvId Matrix;
		SpvId Vector;
	};

	class SpvOpMatrixTimesMatrix : public SpvInstructionBase<SpvOpMatrixTimesMatrix>
	{
	public:
		SpvOpMatrixTimesMatrix(SpvId InResultType, SpvId InLeftMatrix, SpvId InRightMatrix) : SpvInstructionBase(SpvOp::MatrixTimesMatrix)
		, ResultType(InResultType), LeftMatrix(InLeftMatrix), RightMatrix(InRightMatrix)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetLeftMatrix() const { return LeftMatrix; }
		SpvId GetRightMatrix() const { return RightMatrix; }
		
	private:
		SpvId ResultType;
		SpvId LeftMatrix;
		SpvId RightMatrix;
	};

	class SpvOpDot : public SpvInstructionBase<SpvOpDot>
	{
	public:
		SpvOpDot(SpvId InResultType, SpvId InVector1, SpvId InVector2) : SpvInstructionBase(SpvOp::Dot)
		, ResultType(InResultType), Vector1(InVector1), Vector2(InVector2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetVector1() const { return Vector1; }
		SpvId GetVector2() const { return Vector2; }
		
	private:
		SpvId ResultType;
		SpvId Vector1;
		SpvId Vector2;
	};

	class SpvOpAny : public SpvInstructionBase<SpvOpAny>
	{
	public:
		SpvOpAny(SpvId InResultType, SpvId InVector) : SpvInstructionBase(SpvOp::Any)
		, ResultType(InResultType), Vector(InVector)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetVector() const { return Vector; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Vector.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Any;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Vector;
	};

	class SpvOpAll : public SpvInstructionBase<SpvOpAll>
	{
	public:
		SpvOpAll(SpvId InResultType, SpvId InVector) : SpvInstructionBase(SpvOp::All)
		, ResultType(InResultType), Vector(InVector)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetVector() const { return Vector; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Vector.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::All;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Vector;
	};

	class SpvOpIsNan : public SpvInstructionBase<SpvOpIsNan>
	{
	public:
		SpvOpIsNan(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvOp::IsNan)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvOpIsInf : public SpvInstructionBase<SpvOpIsInf>
	{
	public:
		SpvOpIsInf(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvOp::IsInf)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvOpLogicalOr : public SpvInstructionBase<SpvOpLogicalOr>
	{
	public:
		SpvOpLogicalOr(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::LogicalOr)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Operand1.GetValue());
			Bin.Add(Operand2.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::LogicalOr;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpLogicalAnd : public SpvInstructionBase<SpvOpLogicalAnd>
	{
	public:
		SpvOpLogicalAnd(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::LogicalAnd)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Operand1.GetValue());
			Bin.Add(Operand2.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::LogicalAnd;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpLogicalNot : public SpvInstructionBase<SpvOpLogicalNot>
	{
	public:
		SpvOpLogicalNot(SpvId InResultType, SpvId InOperand) : SpvInstructionBase(SpvOp::LogicalNot)
		, ResultType(InResultType), Operand(InOperand)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand() const { return Operand; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Operand.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::LogicalNot;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Operand;
	};

	class SpvOpSelect : public SpvInstructionBase<SpvOpSelect>
	{
	public:
		SpvOpSelect(SpvId InResultType, SpvId InCondition, SpvId InObject1, SpvId InObject2) : SpvInstructionBase(SpvOp::Select)
		, ResultType(InResultType), Condition(InCondition), Object1(InObject1), Object2(InObject2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetCondition() const { return Condition; }
		SpvId GetObject1() const { return Object1; }
		SpvId GetObject2() const { return Object2; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Condition.GetValue());
			Bin.Add(Object1.GetValue());
			Bin.Add(Object2.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Select;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Condition;
		SpvId Object1;
		SpvId Object2;
	};

#define DEFINE_COMPARISON(Name)                                                                                         \
	class SpvOp##Name : public SpvInstructionBase<SpvOp##Name>                                                          \
	{                                                                                                                   \
	public:                                                                                                             \
		SpvOp##Name(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::Name)           \
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)                                          \
		{}                                                                                                              \
		SpvId GetResultType() const { return ResultType; }                                                              \
		SpvId GetOperand1() const { return Operand1; }                                                                  \
		SpvId GetOperand2() const { return Operand2; }                                                                  \
		TArray<uint32> ToBinary() const override                                                                        \
		{                                                                                                               \
			TArray<uint32> Bin;                                                                                         \
			Bin.Add(ResultType.GetValue());                                                                             \
			Bin.Add(GetId().value().GetValue());                                                                        \
			Bin.Add(Operand1.GetValue());                                                                               \
			Bin.Add(Operand2.GetValue());                                                                               \
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Name;                                              \
			Bin.Insert(Header, 0);                                                                                      \
			return Bin;                                                                                                 \
		}                                                                                                               \
	private:                                                                                                            \
		SpvId ResultType;                                                                                               \
		SpvId Operand1;                                                                                                 \
		SpvId Operand2;                                                                                                 \
	};

DEFINE_COMPARISON(IEqual)
DEFINE_COMPARISON(INotEqual)
DEFINE_COMPARISON(UGreaterThan)
DEFINE_COMPARISON(SGreaterThan)
DEFINE_COMPARISON(UGreaterThanEqual)
DEFINE_COMPARISON(SGreaterThanEqual)
DEFINE_COMPARISON(ULessThan)
DEFINE_COMPARISON(SLessThan)
DEFINE_COMPARISON(ULessThanEqual)
DEFINE_COMPARISON(SLessThanEqual)
DEFINE_COMPARISON(FOrdEqual)
DEFINE_COMPARISON(FOrdNotEqual)
DEFINE_COMPARISON(FOrdLessThan)
DEFINE_COMPARISON(FOrdGreaterThan)
DEFINE_COMPARISON(FOrdLessThanEqual)
DEFINE_COMPARISON(FOrdGreaterThanEqual)

	class SpvOpShiftRightLogical : public SpvInstructionBase<SpvOpShiftRightLogical>
	{
	public:
		SpvOpShiftRightLogical(SpvId InResultType, SpvId InBase, SpvId InShift) : SpvInstructionBase(SpvOp::ShiftRightLogical)
		, ResultType(InResultType), Base(InBase), Shift(InShift)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetBase() const { return Base; }
		SpvId GetShift() const { return Shift; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(Base.GetValue());
			Bin.Add(Shift.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::ShiftRightLogical;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId Base;
		SpvId Shift;
	};

	class SpvOpShiftRightArithmetic : public SpvInstructionBase<SpvOpShiftRightArithmetic>
	{
	public:
		SpvOpShiftRightArithmetic(SpvId InResultType, SpvId InBase, SpvId InShift) : SpvInstructionBase(SpvOp::ShiftRightArithmetic)
		, ResultType(InResultType), Base(InBase), Shift(InShift)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetBase() const { return Base; }
		SpvId GetShift() const { return Shift; }
		
	private:
		SpvId ResultType;
		SpvId Base;
		SpvId Shift;
	};

	class SpvOpShiftLeftLogical : public SpvInstructionBase<SpvOpShiftLeftLogical>
	{
	public:
		SpvOpShiftLeftLogical(SpvId InResultType, SpvId InBase, SpvId InShift) : SpvInstructionBase(SpvOp::ShiftLeftLogical)
		, ResultType(InResultType), Base(InBase), Shift(InShift)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetBase() const { return Base; }
		SpvId GetShift() const { return Shift; }
		
	private:
		SpvId ResultType;
		SpvId Base;
		SpvId Shift;
	};

	class SpvOpBitwiseOr : public SpvInstructionBase<SpvOpBitwiseOr>
	{
	public:
		SpvOpBitwiseOr(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::BitwiseOr)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpBitwiseXor : public SpvInstructionBase<SpvOpBitwiseXor>
	{
	public:
		SpvOpBitwiseXor(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::BitwiseXor)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpBitwiseAnd : public SpvInstructionBase<SpvOpBitwiseAnd>
	{
	public:
		SpvOpBitwiseAnd(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::BitwiseAnd)
		, ResultType(InResultType), Operand1(InOperand1), Operand2(InOperand2)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand1() const { return Operand1; }
		SpvId GetOperand2() const { return Operand2; }
		
	private:
		SpvId ResultType;
		SpvId Operand1;
		SpvId Operand2;
	};

	class SpvOpNot : public SpvInstructionBase<SpvOpNot>
	{
	public:
		SpvOpNot(SpvId InResultType, SpvId InOperand) : SpvInstructionBase(SpvOp::Not)
		, ResultType(InResultType), Operand(InOperand)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetOperand() const { return Operand; }
		
	private:
		SpvId ResultType;
		SpvId Operand;
	};

	class SpvOpDPdx : public SpvInstructionBase<SpvOpDPdx>
	{
	public:
		SpvOpDPdx(SpvId InResultType, SpvId InP) : SpvInstructionBase(SpvOp::DPdx)
		, ResultType(InResultType), P(InP)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetP() const { return P; }
		
	private:
		SpvId ResultType;
		SpvId P;
	};

	class SpvOpDPdy : public SpvInstructionBase<SpvOpDPdy>
	{
	public:
		SpvOpDPdy(SpvId InResultType, SpvId InP) : SpvInstructionBase(SpvOp::DPdy)
		, ResultType(InResultType), P(InP)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetP() const { return P; }
		
	private:
		SpvId ResultType;
		SpvId P;
	};

	class SpvOpFwidth : public SpvInstructionBase<SpvOpFwidth>
	{
	public:
		SpvOpFwidth(SpvId InResultType, SpvId InP) : SpvInstructionBase(SpvOp::Fwidth)
		, ResultType(InResultType), P(InP)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetP() const { return P; }
		
	private:
		SpvId ResultType;
		SpvId P;
	};

	class SpvOpBranch : public SpvInstructionBase<SpvOpBranch>
	{
	public:
		SpvOpBranch(SpvId InTargetLabel) : SpvInstructionBase(SpvOp::Branch)
		, TargetLabel(InTargetLabel)
		{}
		
		SpvId GetTargetLabel() const { return TargetLabel; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(TargetLabel.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Branch;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId TargetLabel;
	};

	class SpvOpBranchConditional : public SpvInstructionBase<SpvOpBranchConditional>
	{
	public:
		SpvOpBranchConditional(SpvId InCondition, SpvId InTrueLabel, SpvId InFalseLabel) : SpvInstructionBase(SpvOp::BranchConditional)
		, Condition(InCondition), TrueLabel(InTrueLabel), FalseLabel(InFalseLabel)
		{}
		
		SpvId GetCondition() const { return Condition; }
		SpvId GetTrueLabel() const { return TrueLabel; }
		SpvId GetFalseLabel() const { return FalseLabel; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(Condition.GetValue());
			Bin.Add(TrueLabel.GetValue());
			Bin.Add(FalseLabel.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::BranchConditional;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId Condition;
		SpvId TrueLabel;
		SpvId FalseLabel;
	};

	class SpvOpSwitch : public SpvInstructionBase<SpvOpSwitch>
	{
	public:
		SpvOpSwitch(SpvId InSelector, SpvId InDefault, const TArray<TPair<TArray<uint8>, SpvId>>& InTargets) : SpvInstructionBase(SpvOp::Switch)
		, Selector(InSelector), Default(InDefault), Targets(InTargets)
		{}
		
		SpvId GetSelector() const { return Selector; }
		SpvId GetDefault() const { return Default; }
		const TArray<TPair<TArray<uint8>, SpvId>>& GetTargets() const { return Targets; }
		
	private:
		SpvId Selector;
		SpvId Default;
		TArray<TPair<TArray<uint8>, SpvId>> Targets;
	};

	class SpvOpKill : public SpvInstructionBase<SpvOpKill>
	{
	public:
		SpvOpKill() : SpvInstructionBase(SpvOp::Kill) {}
	};

	class SpvOpReturn : public SpvInstructionBase<SpvOpReturn>
	{
	public:
		SpvOpReturn() : SpvInstructionBase(SpvOp::Return) {}
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::Return;
			Bin.Insert(Header, 0);
			return Bin;
		}
	};

	class SpvOpReturnValue : public SpvInstructionBase<SpvOpReturnValue>
	{
	public:
		SpvOpReturnValue(SpvId InValue) : SpvInstructionBase(SpvOp::ReturnValue)
		, Value(InValue)
		{}
		
		SpvId GetValue() const { return Value; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(Value.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::ReturnValue;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId Value;
	};

	class SpvDebugTypeBasic : public SpvInstructionBase<SpvDebugTypeBasic>
	{
	public:
		SpvDebugTypeBasic(SpvId InResultType, SpvId InExtSet, SpvId InName, SpvId InSize, SpvId InEncoding, SpvId InFlags) : SpvInstructionBase(SpvDebugInfo100::DebugTypeBasic)
		, ResultType(InResultType), ExtSet(InExtSet), Name(InName), Size(InSize), Encoding(InEncoding), Flags(InFlags)
		{}
		
		SpvId GetName() const { return Name; }
		SpvId GetSize() const { return Size; }
		SpvId GetEncoding() const { return Encoding; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(ExtSet.GetValue());
			Bin.Add((uint32)SpvDebugInfo100::DebugTypeBasic);
			Bin.Add(Name.GetValue());
			Bin.Add(Size.GetValue());
			Bin.Add(Encoding.GetValue());
			Bin.Add(Flags.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::ExtInst;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId ExtSet;
		SpvId Name;
		SpvId Size;
		SpvId Encoding;
		SpvId Flags;
	};

	class SpvDebugTypeVector : public SpvInstructionBase<SpvDebugTypeVector>
	{
	public:
		SpvDebugTypeVector(SpvId InResultType, SpvId InExtSet, SpvId InBasicType, SpvId InComponentCount) : SpvInstructionBase(SpvDebugInfo100::DebugTypeVector)
		, ResultType(InResultType), ExtSet(InExtSet), BasicType(InBasicType), ComponentCount(InComponentCount)
		{}
		
		SpvId GetBasicType() const { return BasicType; }
		SpvId GetComponentCount() const { return ComponentCount; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(ExtSet.GetValue());
			Bin.Add((uint32)SpvDebugInfo100::DebugTypeVector);
			Bin.Add(BasicType.GetValue());
			Bin.Add(ComponentCount.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::ExtInst;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId ExtSet;
		SpvId BasicType;
		SpvId ComponentCount;
	};

	class SpvDebugTypeMatrix : public SpvInstructionBase<SpvDebugTypeMatrix>
	{
	public:
		SpvDebugTypeMatrix(SpvId InVectorType, SpvId InVectorCount, SpvId InColumnMajor) : SpvInstructionBase(SpvDebugInfo100::DebugTypeMatrix)
		, VectorType(InVectorType), VectorCount(InVectorCount), ColumnMajor(InColumnMajor)
		{}
		
		SpvId GetVectorType() const { return VectorType; }
		SpvId GetVectorCount() const { return VectorCount; }
		SpvId GetColumnMajor() const { return ColumnMajor; }
		
	private:
		SpvId VectorType;
		SpvId VectorCount;
		SpvId ColumnMajor;
	};

	class SpvDebugTypeComposite : public SpvInstructionBase<SpvDebugTypeComposite>
	{
	public:
		SpvDebugTypeComposite(SpvId InName, SpvId InSize, const TArray<SpvId>& InMembers) : SpvInstructionBase(SpvDebugInfo100::DebugTypeComposite)
		, Name(InName), Size(InSize), Members(InMembers)
		{}
		
		SpvId GetName() const { return Name; }
		SpvId GetSize() const { return Size; }
		const TArray<SpvId>& GetMembers() const { return Members; }
		
	private:
		SpvId Name;
		SpvId Size;
		TArray<SpvId> Members;
	};

	class SpvDebugTypeMember : public SpvInstructionBase<SpvDebugTypeMember>
	{
	public:
		SpvDebugTypeMember(SpvId InName, SpvId InType, SpvId InOffset, SpvId InSize) : SpvInstructionBase(SpvDebugInfo100::DebugTypeMember)
		, Name(InName), Type(InType), Offset(InOffset), Size(InSize)
		{}
		
		SpvId GetName() const { return Name; }
		SpvId GetType() const { return Type; }
		SpvId GetOffset() const { return Offset; }
		SpvId GetSize() const { return Size; }
		
	private:
		SpvId Name;
		SpvId Type;
		SpvId Offset;
		SpvId Size;
	};

	class SpvDebugTypeArray: public SpvInstructionBase<SpvDebugTypeArray>
	{
	public:
		SpvDebugTypeArray(SpvId InBaseType, const TArray<SpvId>& InComponentCounts) : SpvInstructionBase(SpvDebugInfo100::DebugTypeArray)
		, BaseType(InBaseType), ComponentCounts(InComponentCounts)
		{}
		
		SpvId GetBaseType() const { return BaseType; }
		const TArray<SpvId>& GetComponentCounts() const { return ComponentCounts; }
		
	private:
		SpvId BaseType;
		TArray<SpvId> ComponentCounts;
	};

	class SpvDebugTypeFunction : public SpvInstructionBase<SpvDebugTypeFunction>
	{
	public:
		SpvDebugTypeFunction(SpvId InReturnType, const TArray<SpvId>& InParmTypes) : SpvInstructionBase(SpvDebugInfo100::DebugTypeFunction)
		, ReturnType(InReturnType), ParmTypes(InParmTypes)
		{}
		
		SpvId GetReturnType() const { return ReturnType; }
		const TArray<SpvId>& GetParmTypes() const { return ParmTypes; }
		
	private:
		SpvId ReturnType;
		TArray<SpvId> ParmTypes;
	};

	class SpvDebugCompilationUnit : public SpvInstructionBase<SpvDebugCompilationUnit>
	{
	public:
		SpvDebugCompilationUnit() : SpvInstructionBase(SpvDebugInfo100::DebugCompilationUnit) {}
	};

	class SpvDebugLexicalBlock : public SpvInstructionBase<SpvDebugLexicalBlock>
	{
	public:
		SpvDebugLexicalBlock(SpvId InSource, SpvId InLine, SpvId InParent) : SpvInstructionBase(SpvDebugInfo100::DebugLexicalBlock)
		, Source(InSource), Line(InLine), Parent(InParent)
		{}
		
		SpvId GetSource() const { return Source; }
		SpvId GetLine() const { return Line; }
		SpvId GetParentId() const { return Parent; }
		
	private:
		SpvId Source;
		SpvId Line;
		SpvId Parent;
	};

	class SpvDebugFunction : public SpvInstructionBase<SpvDebugFunction>
	{
	public:
		SpvDebugFunction(SpvId InName, SpvId InTypeDesc, SpvId InSource, SpvId InLine, SpvId InParent, SpvId InScopeLine)
		: SpvInstructionBase(SpvDebugInfo100::DebugFunction)
		, Name(InName), TypeDesc(InTypeDesc)
		, Source(InSource), Line(InLine), Parent(InParent)
		, ScopeLine(InScopeLine)
		{}
		
		SpvId GetNameId() const { return Name; }
		SpvId GetTypeDescId() const { return TypeDesc; }
		SpvId GetSource() const { return Source; }
		SpvId GetLine() const { return Line; }
		SpvId GetParentId() const { return Parent; }
		SpvId GetScopeLine() const { return ScopeLine; }
		
	private:
		SpvId Name;
		SpvId TypeDesc;
		SpvId Source;
		SpvId Line;
		SpvId Parent;
		SpvId ScopeLine;
	};

	class SpvDebugScope : public SpvInstructionBase<SpvDebugScope>
	{
	public:
		SpvDebugScope(SpvId InScope, std::optional<SpvId> InInlined) : SpvInstructionBase(SpvDebugInfo100::DebugScope)
		, Scope(InScope), Inlined(InInlined)
		{}
		
		SpvId GetScope() const { return Scope; }
		std::optional<SpvId> GetInlined() const { return Inlined; }
		
	private:
		SpvId Scope;
		std::optional<SpvId> Inlined;
	};

	class SpvDebugSource : public SpvInstructionBase<SpvDebugSource>
	{
	public:
		SpvDebugSource(SpvId InFile) : SpvInstructionBase(SpvDebugInfo100::DebugSource)
		, File(InFile)
		{}
		
		SpvId GetFile() const { return File; }
		
	private:
		SpvId File;
	};

	class SpvDebugLine : public SpvInstructionBase<SpvDebugLine>
	{
	public:
		SpvDebugLine(SpvId InSource, SpvId InLineStart) : SpvInstructionBase(SpvDebugInfo100::DebugLine)
		, Source(InSource), LineStart(InLineStart)
		{}
		
		SpvId GetSource() const { return Source; }
		SpvId GetLineStart() const { return LineStart; }
		
	private:
		SpvId Source;
		SpvId LineStart;
	};

	class SpvDebugDeclare : public SpvInstructionBase<SpvDebugDeclare>
	{
	public:
		SpvDebugDeclare(SpvId InVarDesc, SpvId InVariable) : SpvInstructionBase(SpvDebugInfo100::DebugDeclare)
		, VarDesc(InVarDesc)
		, Variable(InVariable)
		{}
		
		SpvId GetVarDesc() const { return VarDesc; }
		SpvId GetVariable() const { return Variable; }
		
	private:
		SpvId VarDesc;
		SpvId Variable;
	};

	class SpvDebugValue : public SpvInstructionBase<SpvDebugValue>
	{
	public:
		SpvDebugValue(SpvId InLocalVariable, SpvId InValue) : SpvInstructionBase(SpvDebugInfo100::DebugValue)
		, LocalVariable(InLocalVariable), Value(InValue)
		{}

		SpvId GetLocalVariable() const { return LocalVariable; }
		SpvId GetValue() const { return Value; }

	private:
		SpvId LocalVariable;
		SpvId Value;
	};

	class SpvDebugInlinedAt : public SpvInstructionBase<SpvDebugInlinedAt>
	{
	public:
		SpvDebugInlinedAt(SpvId InLine, SpvId InScope, std::optional<SpvId> InInlined) : SpvInstructionBase(SpvDebugInfo100::DebugInlinedAt)
		, Line(InLine), Scope(InScope), Inlined(InInlined)
		{}

		SpvId GetLine() const { return Line; }
		SpvId GetScope() const { return Scope; }
		std::optional<SpvId> GetInlined() const { return Inlined; }

	private:
		SpvId Line;
		SpvId Scope;
		std::optional<SpvId> Inlined;
	};

	class SpvDebugLocalVariable : public SpvInstructionBase<SpvDebugLocalVariable>
	{
	public:
		SpvDebugLocalVariable(SpvId InName, SpvId InTypeDesc, SpvId InLine, SpvId InParent)
		: SpvInstructionBase(SpvDebugInfo100::DebugLocalVariable)
		, Name(InName), TypeDesc(InTypeDesc)
		, Line(InLine), Parent(InParent)
		{}
		
		SpvId GetNameId() const { return Name; }
		SpvId GetTypeDescId() const { return TypeDesc; }
		SpvId GetLine() const { return Line; }
		SpvId GetParentId() const { return Parent; }
		
	private:
		SpvId Name;
		SpvId TypeDesc;
		SpvId Line;
		SpvId Parent;
	};

	class SpvDebugFunctionDefinition : public SpvInstructionBase<SpvDebugFunctionDefinition>
	{
	public:
		SpvDebugFunctionDefinition(SpvId InFunction, SpvId InDefinition) : SpvInstructionBase(SpvDebugInfo100::DebugFunctionDefinition)
		, Function(InFunction), Definition(InDefinition)
		{}

		SpvId GetFunction() const { return Function; }
		SpvId GetDefinition() const { return Definition; }

	private:
		SpvId Function;
		SpvId Definition;
	};

	class SpvDebugTypeTemplate : public SpvInstructionBase<SpvDebugTypeTemplate>
	{
	public:
		SpvDebugTypeTemplate(SpvId InTarget, const TArray<SpvId>& InParameters) : SpvInstructionBase(SpvDebugInfo100::DebugTypeTemplate)
		, Target(InTarget), Parameters(InParameters)
		{}

		SpvId GetTarget() const { return Target; }
		const TArray<SpvId>& GetParameters() const { return Parameters; }

	private:
		SpvId Target;
		TArray<SpvId> Parameters;
	};

	class SpvDebugGlobalVariable : public SpvInstructionBase<SpvDebugGlobalVariable>
	{
	public:
		SpvDebugGlobalVariable(SpvId InName, SpvId InTypeDesc, SpvId InLine, SpvId InParent, SpvId InVar)
		: SpvInstructionBase(SpvDebugInfo100::DebugGlobalVariable)
		, Name(InName), TypeDesc(InTypeDesc), Line(InLine), Parent(InParent), Var(InVar)
		{}
		
		SpvId GetNameId() const { return Name; }
		SpvId GetTypeDescId() const { return TypeDesc; }
		SpvId GetLine() const { return Line; }
		SpvId GetParentId() const { return Parent; }
		SpvId GetVarId() const { return Var; }
		
	private:
		SpvId Name;
		SpvId TypeDesc;
		SpvId Line;
		SpvId Parent;
		SpvId Var;
	};

	class SpvRoundEven : public SpvInstructionBase<SpvRoundEven>
	{
	public:
		SpvRoundEven(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::RoundEven)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvTrunc : public SpvInstructionBase<SpvTrunc>
	{
	public:
		SpvTrunc(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Trunc)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvFAbs : public SpvInstructionBase<SpvFAbs>
	{
	public:
		SpvFAbs(SpvId InResultType, SpvId InExtSet, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::FAbs)
		, ResultType(InResultType), ExtSet(InExtSet), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		TArray<uint32> ToBinary() const override
		{
			TArray<uint32> Bin;
			Bin.Add(ResultType.GetValue());
			Bin.Add(GetId().value().GetValue());
			Bin.Add(ExtSet.GetValue());
			Bin.Add((uint32)SpvGLSLstd450::FAbs);
			Bin.Add(X.GetValue());
			uint32 Header = ((Bin.Num() + 1) << 16) | (uint32)SpvOp::ExtInst;
			Bin.Insert(Header, 0);
			return Bin;
		}
		
	private:
		SpvId ResultType;
		SpvId ExtSet;
		SpvId X;
	};

	class SpvSAbs : public SpvInstructionBase<SpvSAbs>
	{
	public:
		SpvSAbs(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::SAbs)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvFSign : public SpvInstructionBase<SpvFSign>
	{
	public:
		SpvFSign(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::FSign)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvSSign : public SpvInstructionBase<SpvSSign>
	{
	public:
		SpvSSign(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::SSign)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvFloor : public SpvInstructionBase<SpvFloor>
	{
	public:
		SpvFloor(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Floor)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvCeil : public SpvInstructionBase<SpvCeil>
	{
	public:
		SpvCeil(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Ceil)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvFract : public SpvInstructionBase<SpvFract>
	{
	public:
		SpvFract(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Fract)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvSin : public SpvInstructionBase<SpvSin>
	{
	public:
		SpvSin(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Sin)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvCos : public SpvInstructionBase<SpvCos>
	{
	public:
		SpvCos(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Cos)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvTan : public SpvInstructionBase<SpvTan>
	{
	public:
		SpvTan(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Tan)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvAsin : public SpvInstructionBase<SpvAsin>
	{
	public:
		SpvAsin(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Asin)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvAcos : public SpvInstructionBase<SpvAcos>
	{
	public:
		SpvAcos(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Acos)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvAtan : public SpvInstructionBase<SpvAtan>
	{
	public:
		SpvAtan(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Atan)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvAtan2 : public SpvInstructionBase<SpvAtan2>
	{
	public:
		SpvAtan2(SpvId InResultType, SpvId InY, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Atan2)
			, ResultType(InResultType), Y(InY), X(InX)
		{
		}

		SpvId GetResultType() const { return ResultType; }
		SpvId GetY() const { return Y; }
		SpvId GetX() const { return X; }

	private:
		SpvId ResultType;
		SpvId Y;
		SpvId X;
	};

	class SpvPow : public SpvInstructionBase<SpvPow>
	{
	public:
		SpvPow(SpvId InResultType, SpvId InX, SpvId InY) : SpvInstructionBase(SpvGLSLstd450::Pow)
		, ResultType(InResultType), X(InX), Y(InY)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetY() const { return Y; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId Y;
	};

	class SpvExp : public SpvInstructionBase<SpvExp>
	{
	public:
		SpvExp(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Exp)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvLog : public SpvInstructionBase<SpvLog>
	{
	public:
		SpvLog(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Log)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvExp2 : public SpvInstructionBase<SpvExp2>
	{
	public:
		SpvExp2(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Exp2)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvLog2 : public SpvInstructionBase<SpvLog2>
	{
	public:
		SpvLog2(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Log2)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvSqrt : public SpvInstructionBase<SpvSqrt>
	{
	public:
		SpvSqrt(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Sqrt)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvInverseSqrt : public SpvInstructionBase<SpvInverseSqrt>
	{
	public:
		SpvInverseSqrt(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::InverseSqrt)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvDeterminant : public SpvInstructionBase<SpvDeterminant>
	{
	public:
		SpvDeterminant(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Determinant)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvUMin : public SpvInstructionBase<SpvUMin>
	{
	public:
		SpvUMin(SpvId InResultType, SpvId InX, SpvId InY) : SpvInstructionBase(SpvGLSLstd450::UMin)
		, ResultType(InResultType), X(InX), Y(InY)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetY() const { return Y; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId Y;
	};

	class SpvSMin : public SpvInstructionBase<SpvSMin>
	{
	public:
		SpvSMin(SpvId InResultType, SpvId InX, SpvId InY) : SpvInstructionBase(SpvGLSLstd450::SMin)
		, ResultType(InResultType), X(InX), Y(InY)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetY() const { return Y; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId Y;
	};

	class SpvUMax : public SpvInstructionBase<SpvUMax>
	{
	public:
		SpvUMax(SpvId InResultType, SpvId InX, SpvId InY) : SpvInstructionBase(SpvGLSLstd450::UMax)
		, ResultType(InResultType), X(InX), Y(InY)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetY() const { return Y; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId Y;
	};

	class SpvSMax : public SpvInstructionBase<SpvSMax>
	{
	public:
		SpvSMax(SpvId InResultType, SpvId InX, SpvId InY) : SpvInstructionBase(SpvGLSLstd450::SMax)
		, ResultType(InResultType), X(InX), Y(InY)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetY() const { return Y; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId Y;
	};

	class SpvFClamp : public SpvInstructionBase<SpvFClamp>
	{
	public:
		SpvFClamp(SpvId InResultType, SpvId InX, SpvId InMinVal, SpvId InMaxVal) : SpvInstructionBase(SpvGLSLstd450::FClamp)
		, ResultType(InResultType), X(InX), MinVal(InMinVal), MaxVal(InMaxVal)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetMinVal() const { return MinVal; }
		SpvId GetMaxVal() const { return MaxVal; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId MinVal;
		SpvId MaxVal;
	};

	class SpvUClamp : public SpvInstructionBase<SpvUClamp>
	{
	public:
		SpvUClamp(SpvId InResultType, SpvId InX, SpvId InMinVal, SpvId InMaxVal) : SpvInstructionBase(SpvGLSLstd450::UClamp)
		, ResultType(InResultType), X(InX), MinVal(InMinVal), MaxVal(InMaxVal)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetMinVal() const { return MinVal; }
		SpvId GetMaxVal() const { return MaxVal; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId MinVal;
		SpvId MaxVal;
	};

	class SpvSClamp : public SpvInstructionBase<SpvSClamp>
	{
	public:
		SpvSClamp(SpvId InResultType, SpvId InX, SpvId InMinVal, SpvId InMaxVal) : SpvInstructionBase(SpvGLSLstd450::SClamp)
		, ResultType(InResultType), X(InX), MinVal(InMinVal), MaxVal(InMaxVal)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetMinVal() const { return MinVal; }
		SpvId GetMaxVal() const { return MaxVal; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId MinVal;
		SpvId MaxVal;
	};

	class SpvFMix : public SpvInstructionBase<SpvFMix>
	{
	public:
		SpvFMix(SpvId InResultType, SpvId InX, SpvId InY, SpvId InA) : SpvInstructionBase(SpvGLSLstd450::FMix)
		, ResultType(InResultType), X(InX), Y(InY), A(InA)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetY() const { return Y; }
		SpvId GetA() const { return A; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId Y;
		SpvId A;
	};

	class SpvStep : public SpvInstructionBase<SpvStep>
	{
	public:
		SpvStep(SpvId InResultType, SpvId InEdge, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Step)
		, ResultType(InResultType), Edge(InEdge), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetEdge() const { return Edge; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId Edge;
		SpvId X;
	};

	class SpvSmoothStep : public SpvInstructionBase<SpvSmoothStep>
	{
	public:
		SpvSmoothStep(SpvId InResultType, SpvId InEdge0, SpvId InEdge1, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::SmoothStep)
		, ResultType(InResultType), Edge0(InEdge0), Edge1(InEdge1), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetEdge0() const { return Edge0; }
		SpvId GetEdge1() const { return Edge1; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId Edge0;
		SpvId Edge1;
		SpvId X;
	};

	class SpvPackHalf2x16 : public SpvInstructionBase<SpvPackHalf2x16>
	{
	public:
		SpvPackHalf2x16(SpvId InResultType, SpvId InV) : SpvInstructionBase(SpvGLSLstd450::PackHalf2x16)
		, ResultType(InResultType), V(InV)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetV() const { return V; }
		
	private:
		SpvId ResultType;
		SpvId V;
	};

	class SpvUnpackHalf2x16 : public SpvInstructionBase<SpvUnpackHalf2x16>
	{
	public:
		SpvUnpackHalf2x16(SpvId InResultType, SpvId InV) : SpvInstructionBase(SpvGLSLstd450::UnpackHalf2x16)
		, ResultType(InResultType), V(InV)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetV() const { return V; }
		
	private:
		SpvId ResultType;
		SpvId V;
	};

	class SpvLength : public SpvInstructionBase<SpvLength>
	{
	public:
		SpvLength(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Length)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvDistance : public SpvInstructionBase<SpvDistance>
	{
	public:
		SpvDistance(SpvId InResultType, SpvId InP0, SpvId InP1) : SpvInstructionBase(SpvGLSLstd450::Distance)
		, ResultType(InResultType), P0(InP0), P1(InP1)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetP0() const { return P0; }
		SpvId GetP1() const { return P1; }
		
	private:
		SpvId ResultType;
		SpvId P0;
		SpvId P1;
	};

	class SpvCross : public SpvInstructionBase<SpvCross>
	{
	public:
		SpvCross(SpvId InResultType, SpvId InX, SpvId InY) : SpvInstructionBase(SpvGLSLstd450::Cross)
		, ResultType(InResultType), X(InX), Y(InY)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetY() const { return Y; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId Y;
	};

	class SpvNormalize : public SpvInstructionBase<SpvNormalize>
	{
	public:
		SpvNormalize(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::Normalize)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
		SpvId X;
	};

	class SpvReflect : public SpvInstructionBase<SpvReflect>
	{
	public:
		SpvReflect(SpvId InResultType, SpvId InI, SpvId InN) : SpvInstructionBase(SpvGLSLstd450::Reflect)
		, ResultType(InResultType), I(InI), N(InN)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetI() const { return I; }
		SpvId GetN() const { return N; }
		
	private:
		SpvId ResultType;
		SpvId I;
		SpvId N;
	};

	class SpvRefract : public SpvInstructionBase<SpvRefract>
	{
	public:
		SpvRefract(SpvId InResultType, SpvId InI, SpvId InN, SpvId InEta) : SpvInstructionBase(SpvGLSLstd450::Refract)
		, ResultType(InResultType), I(InI), N(InN), Eta(InEta)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetI() const { return I; }
		SpvId GetN() const { return N; }
		SpvId GetEta() const { return Eta; }
		
	private:
		SpvId ResultType;
		SpvId I;
		SpvId N;
		SpvId Eta;
	};

	class SpvNMin : public SpvInstructionBase<SpvNMin>
	{
	public:
		SpvNMin(SpvId InResultType, SpvId InX, SpvId InY) : SpvInstructionBase(SpvGLSLstd450::NMin)
		, ResultType(InResultType), X(InX), Y(InY)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetY() const { return Y; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId Y;
	};

	class SpvNMax : public SpvInstructionBase<SpvNMax>
	{
	public:
		SpvNMax(SpvId InResultType, SpvId InX, SpvId InY) : SpvInstructionBase(SpvGLSLstd450::NMax)
		, ResultType(InResultType), X(InX), Y(InY)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		SpvId GetY() const { return Y; }
		
	private:
		SpvId ResultType;
		SpvId X;
		SpvId Y;
	};

}
