#pragma once
#include "SpirvCore.h"
#include "SpirvExt.h"

namespace FW
{
	class SpvInstruction;
	class SpvVisitor
	{
	public:
		virtual void Parse(const TArray<TUniquePtr<SpvInstruction>>& Insts) {}
		
		virtual void Visit(SpvInstruction*) {}
		
		//Core
		virtual void Visit(class SpvOpEntryPoint* Inst) {}
		virtual void Visit(class SpvOpExecutionMode* Inst) {}
		virtual void Visit(class SpvOpString* Inst) {}
		virtual void Visit(class SpvOpDecorate* Inst) {}
		virtual void Visit(class SpvOpMemberDecorate* Inst) {}
		virtual void Visit(class SpvOpTypeVoid* Inst) {}
		virtual void Visit(class SpvOpTypeBool* Inst) {}
		virtual void Visit(class SpvOpTypeFloat* Inst) {}
		virtual void Visit(class SpvOpTypeInt* Inst) {}
		virtual void Visit(class SpvOpTypeVector* Inst) {}
		virtual void Visit(class SpvOpTypePointer* Inst) {}
		virtual void Visit(class SpvOpTypeStruct* Inst) {}
		virtual void Visit(class SpvOpTypeImage* Inst) {}
		virtual void Visit(class SpvOpTypeSampler* Inst) {}
		virtual void Visit(class SpvOpTypeArray* Inst) {}
		virtual void Visit(class SpvOpTypeRuntimeArray* Inst) {}
		
		virtual void Visit(class SpvOpConstant* Inst) {}
		virtual void Visit(class SpvOpConstantTrue* Inst) {}
		virtual void Visit(class SpvOpConstantFalse* Inst) {}
		virtual void Visit(class SpvOpConstantComposite* Inst) {}
		virtual void Visit(class SpvOpConstantNull* Inst) {}
		virtual void Visit(class SpvOpFunction* Inst) {}
		virtual void Visit(class SpvOpFunctionParameter* Inst) {}
		virtual void Visit(class SpvOpFunctionCall* Inst) {}
		virtual void Visit(class SpvOpVariable* Inst) {}
		virtual void Visit(class SpvOpLabel* Inst) {}
		virtual void Visit(class SpvOpLoad* Inst) {}
		virtual void Visit(class SpvOpStore* Inst) {}
		virtual void Visit(class SpvOpVectorShuffle* Inst) {}
		virtual void Visit(class SpvOpCompositeConstruct* Inst) {}
		virtual void Visit(class SpvOpCompositeExtract* Inst) {}
		virtual void Visit(class SpvOpAccessChain* Inst) {}
		virtual void Visit(class SpvOpSampledImage* Inst) {}
		virtual void Visit(class SpvOpImageSampleImplicitLod* Inst) {}
		virtual void Visit(class SpvOpVectorTimesScalar* Inst) {}
		virtual void Visit(class SpvOpDot* Inst) {}
		virtual void Visit(class SpvOpAny* Inst) {}
		virtual void Visit(class SpvOpAll* Inst) {}
		virtual void Visit(class SpvOpIsNan* Inst) {}
		virtual void Visit(class SpvOpIsInf* Inst) {}
		virtual void Visit(class SpvOpLogicalOr* Inst) {}
		virtual void Visit(class SpvOpLogicalAnd* Inst) {}
		virtual void Visit(class SpvOpLogicalNot* Inst) {}
		virtual void Visit(class SpvOpSelect* Inst) {}
		virtual void Visit(class SpvOpIEqual* Inst) {}
		virtual void Visit(class SpvOpINotEqual* Inst) {}
		virtual void Visit(class SpvOpSGreaterThan* Inst) {}
		virtual void Visit(class SpvOpSLessThan* Inst) {}
		virtual void Visit(class SpvOpFOrdLessThan* Inst) {}
		virtual void Visit(class SpvOpFOrdGreaterThan* Inst) {}
		virtual void Visit(class SpvOpBitwiseOr* Inst) {}
		virtual void Visit(class SpvOpBitwiseXor* Inst) {}
		virtual void Visit(class SpvOpBitwiseAnd* Inst) {}
		virtual void Visit(class SpvOpConvertFToU* Inst) {}
		virtual void Visit(class SpvOpConvertFToS* Inst) {}
		virtual void Visit(class SpvOpConvertSToF* Inst) {}
		virtual void Visit(class SpvOpConvertUToF* Inst) {}
		virtual void Visit(class SpvOpFNegate* Inst) {}
		virtual void Visit(class SpvOpIAdd* Inst) {}
		virtual void Visit(class SpvOpFAdd* Inst) {}
		virtual void Visit(class SpvOpISub* Inst) {}
		virtual void Visit(class SpvOpFSub* Inst) {}
		virtual void Visit(class SpvOpIMul* Inst) {}
		virtual void Visit(class SpvOpFMul* Inst) {}
		virtual void Visit(class SpvOpUDiv* Inst) {}
		virtual void Visit(class SpvOpSDiv* Inst) {}
		virtual void Visit(class SpvOpFDiv* Inst) {}
		virtual void Visit(class SpvOpUMod* Inst) {}
		virtual void Visit(class SpvOpSRem* Inst) {}
		virtual void Visit(class SpvOpFRem* Inst) {}
		virtual void Visit(class SpvOpNot* Inst) {}
		virtual void Visit(class SpvOpDPdx* Inst) {}
		virtual void Visit(class SpvOpDPdy* Inst) {}
		virtual void Visit(class SpvOpBranch* Inst) {}
		virtual void Visit(class SpvOpBranchConditional* Inst) {}
		virtual void Visit(class SpvOpReturn* Inst) {}
		virtual void Visit(class SpvOpReturnValue* Inst) {}
		
		//NonSemantic.Shader.DebugInfo.100
		virtual void Visit(class SpvDebugTypeBasic* Inst) {}
		virtual void Visit(class SpvDebugTypeVector* Inst) {}
		virtual void Visit(class SpvDebugTypeComposite* Inst) {}
		virtual void Visit(class SpvDebugTypeMember* Inst) {}
		virtual void Visit(class SpvDebugTypeArray* Inst) {}
		virtual void Visit(class SpvDebugTypeFunction* Inst) {}
		virtual void Visit(class SpvDebugCompilationUnit* Inst) {}
		virtual void Visit(class SpvDebugLexicalBlock* Inst) {}
		virtual void Visit(class SpvDebugFunction* Inst) {}
		virtual void Visit(class SpvDebugLine* Inst) {}
		virtual void Visit(class SpvDebugScope* Inst) {}
		virtual void Visit(class SpvDebugDeclare* Inst) {}
		virtual void Visit(class SpvDebugLocalVariable* Inst) {}
		virtual void Visit(class SpvDebugGlobalVariable* Inst) {}
		
		//GLSL.std.450
		virtual void Visit(class SpvRoundEven* Inst) {}
		virtual void Visit(class SpvFAbs* Inst) {}
		virtual void Visit(class SpvSAbs* Inst) {}
		virtual void Visit(class SpvFloor* Inst) {}
		virtual void Visit(class SpvCeil* Inst) {}
		virtual void Visit(class SpvFract* Inst) {}
		virtual void Visit(class SpvSin* Inst) {}
		virtual void Visit(class SpvCos* Inst) {}
		virtual void Visit(class SpvTan* Inst) {}
		virtual void Visit(class SpvAsin* Inst) {}
		virtual void Visit(class SpvAcos* Inst) {}
		virtual void Visit(class SpvAtan* Inst) {}
		virtual void Visit(class SpvPow* Inst) {}
		virtual void Visit(class SpvExp* Inst) {}
		virtual void Visit(class SpvLog* Inst) {}
		virtual void Visit(class SpvSqrt* Inst) {}
		virtual void Visit(class SpvUMin* Inst) {}
		virtual void Visit(class SpvSMin* Inst) {}
		virtual void Visit(class SpvUMax* Inst) {}
		virtual void Visit(class SpvSMax* Inst) {}
		virtual void Visit(class SpvFClamp* Inst) {}
		virtual void Visit(class SpvUClamp* Inst) {}
		virtual void Visit(class SpvSClamp* Inst) {}
		virtual void Visit(class SpvFMix* Inst) {}
		virtual void Visit(class SpvStep* Inst) {}
		virtual void Visit(class SpvSmoothStep* Inst) {}
		virtual void Visit(class SpvLength* Inst) {}
		virtual void Visit(class SpvDistance* Inst) {}
		virtual void Visit(class SpvNormalize* Inst) {}
		virtual void Visit(class SpvNMin* Inst) {}
		virtual void Visit(class SpvNMax* Inst) {}
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
		virtual void Accpet(SpvVisitor* Visitor) = 0;
		
	private:
		SpvInstKind Kind;
		std::optional<SpvId> ResultId;
	};

	template<typename T>
	class SpvInstructionBase : public SpvInstruction
	{
	public:
		using SpvInstruction::SpvInstruction;
		
		void Accpet(SpvVisitor* Visitor) override
		{
			Visitor->Visit(static_cast<T*>(this));
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
		SpvOpDecorate(SpvId InTarget, SpvDecorationKind InDecorationKind, const TArray<uint8>& InOperands)
		: SpvInstructionBase(SpvOp::Decorate)
		, Target(InTarget)
		, DecorationKind(InDecorationKind)
		, Operands(InOperands)
		{}
		
		SpvId GetTargetId() const { return Target; }
		SpvDecorationKind GetKind() const { return DecorationKind; }
		const TArray<uint8>& GetExtraOperands() const { return Operands; }
		
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
		
		SpvId GetComponentTypeId() const { return ComponentType; }
		uint32 GetComponentCount() const { return ComponentCount; }
		
	private:
		SpvId ComponentType;
		uint32 ComponentCount;
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
		
	private:
		SpvStorageClass StorageClass;
		SpvId PointeeType;
	};

	class SpvOpTypeStruct : public SpvInstructionBase<SpvOpTypeStruct>
	{
	public:
		SpvOpTypeStruct(const TArray<SpvId>& InMemberTypes)
		: SpvInstructionBase(SpvOp::TypeStruct)
		, MemberTypes(InMemberTypes)
		{}
		
		const TArray<SpvId>& GetMemberTypeIds() const { return MemberTypes; }
	
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

	class SpvOpString : public SpvInstructionBase<SpvOpString>
	{
	public:
		SpvOpString(const FString& InStr)
		: SpvInstructionBase(SpvOp::String)
		, Str(InStr)
		{}
		
		const FString& GetStr() const { return Str; }
		
	private:
		FString Str;
	};

	class SpvOpFunction : public SpvInstructionBase<SpvOpFunction>
	{
	public:
		SpvOpFunction(SpvId InResultType, SpvId InFunctionType) : SpvInstructionBase(SpvOp::Function)
		, ResultType(InResultType)
		, FunctionType(InFunctionType)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetFunctionType() const { return FunctionType; }
		
	private:
		SpvId ResultType;
		SpvId FunctionType;
	};

	class SpvOpFunctionParameter : public SpvInstructionBase<SpvOpFunctionParameter>
	{
	public:
		SpvOpFunctionParameter(SpvId InResultType) : SpvInstructionBase(SpvOp::FunctionParameter)
		, ResultType(InResultType)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		
	private:
		SpvId ResultType;
	};

	class SpvOpFunctionCall : public SpvInstructionBase<SpvOpFunctionCall>
	{
	public:
		SpvOpFunctionCall(SpvId InResultType, SpvId InFunction, const TArray<SpvId>& InArguments) : SpvInstructionBase(SpvOp::FunctionCall)
		, ResultType(InResultType)
		, Function(InFunction)
		, Arguments(InArguments)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetFunction() const { return Function; }
		const TArray<SpvId>& GetArguments() const { return Arguments; }
		
	private:
		SpvId ResultType;
		SpvId Function;
		TArray<SpvId> Arguments;
	};

	class SpvOpVariable : public SpvInstructionBase<SpvOpVariable>
	{
	public:
		SpvOpVariable(SpvId InResultType, SpvStorageClass InStorageClass, std::optional<SpvId> InInitializer) : SpvInstructionBase(SpvOp::Variable)
		, ResultType(InResultType)
		, StorageClass(InStorageClass)
		, Initializer(InInitializer)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvStorageClass GetStorageClass() const { return StorageClass; }
		std::optional<SpvId> GetInitializer() const { return Initializer; }
		
	private:
		SpvId ResultType;
		SpvStorageClass StorageClass;
		std::optional<SpvId> Initializer;
	};

	class SpvOpLabel : public SpvInstructionBase<SpvOpLabel>
	{
	public:
		SpvOpLabel() : SpvInstructionBase(SpvOp::Label) {}
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
		
	private:
		SpvId ResultType;
		SpvId Composite;
		TArray<uint32> Indexes;
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

	class SpvOpConvertFToU : public SpvInstructionBase<SpvOpConvertFToU>
	{
	public:
		SpvOpConvertFToU(SpvId InResultType, SpvId InFloatValue) : SpvInstructionBase(SpvOp::ConvertFToU)
		, ResultType(InResultType), FloatValue(InFloatValue)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetFloatValue() const { return FloatValue; }
		
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
		
	private:
		SpvId ResultType;
		SpvId Condition;
		SpvId Object1;
		SpvId Object2;
	};

	class SpvOpIEqual : public SpvInstructionBase<SpvOpIEqual>
	{
	public:
		SpvOpIEqual(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::IEqual)
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

	class SpvOpINotEqual : public SpvInstructionBase<SpvOpINotEqual>
	{
	public:
		SpvOpINotEqual(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::INotEqual)
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

	class SpvOpSGreaterThan : public SpvInstructionBase<SpvOpSGreaterThan>
	{
	public:
		SpvOpSGreaterThan(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::SGreaterThan)
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

	class SpvOpSLessThan : public SpvInstructionBase<SpvOpSLessThan>
	{
	public:
		SpvOpSLessThan(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::SLessThan)
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

	class SpvOpFOrdLessThan : public SpvInstructionBase<SpvOpFOrdLessThan>
	{
	public:
		SpvOpFOrdLessThan(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::FOrdLessThan)
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

	class SpvOpFOrdGreaterThan : public SpvInstructionBase<SpvOpFOrdGreaterThan>
	{
	public:
		SpvOpFOrdGreaterThan(SpvId InResultType, SpvId InOperand1, SpvId InOperand2) : SpvInstructionBase(SpvOp::FOrdGreaterThan)
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

	class SpvOpBranch : public SpvInstructionBase<SpvOpBranch>
	{
	public:
		SpvOpBranch(SpvId InTargetLabel) : SpvInstructionBase(SpvOp::Branch)
		, TargetLabel(InTargetLabel)
		{}
		
		SpvId GetTargetLabel() const { return TargetLabel; }
		
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
		
	private:
		SpvId Condition;
		SpvId TrueLabel;
		SpvId FalseLabel;
	};

	class SpvOpReturn : public SpvInstructionBase<SpvOpReturn>
	{
	public:
		SpvOpReturn() : SpvInstructionBase(SpvOp::Return) {}
	};

	class SpvOpReturnValue : public SpvInstructionBase<SpvOpReturnValue>
	{
	public:
		SpvOpReturnValue(SpvId InValue) : SpvInstructionBase(SpvOp::ReturnValue)
		, Value(InValue)
		{}
		
		SpvId GetValue() const { return Value; }
		
	private:
		SpvId Value;
	};

	class SpvDebugTypeBasic : public SpvInstructionBase<SpvDebugTypeBasic>
	{
	public:
		SpvDebugTypeBasic(SpvId InName, SpvId InSize, SpvId InEncoding) : SpvInstructionBase(SpvDebugInfo100::DebugTypeBasic)
		, Name(InName), Size(InSize), Encoding(InEncoding)
		{}
		
		SpvId GetName() const { return Name; }
		SpvId GetSize() const { return Size; }
		SpvId GetEncoding() const { return Encoding; }
		
	private:
		SpvId Name;
		SpvId Size;
		SpvId Encoding;
	};

	class SpvDebugTypeVector : public SpvInstructionBase<SpvDebugTypeVector>
	{
	public:
		SpvDebugTypeVector(SpvId InBasicType, SpvId InComponentCount) : SpvInstructionBase(SpvDebugInfo100::DebugTypeVector)
		, BasicType(InBasicType), ComponentCount(InComponentCount)
		{}
		
		SpvId GetBasicType() const { return BasicType; }
		SpvId GetComponentCount() const { return ComponentCount; }
		
	private:
		SpvId BasicType;
		SpvId ComponentCount;
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
		SpvDebugLexicalBlock(SpvId InLine, SpvId InParent) : SpvInstructionBase(SpvDebugInfo100::DebugLexicalBlock)
		, Line(InLine), Parent(InParent)
		{}
		
		SpvId GetLine() const { return Line; }
		SpvId GetParentId() const { return Parent; }
		
	private:
		SpvId Line;
		SpvId Parent;
	};

	class SpvDebugFunction : public SpvInstructionBase<SpvDebugFunction>
	{
	public:
		SpvDebugFunction(SpvId InName, SpvId InTypeDesc, SpvId InLine, SpvId InParent, SpvId InScopeLine)
		: SpvInstructionBase(SpvDebugInfo100::DebugFunction)
		, Name(InName), TypeDesc(InTypeDesc)
		, Line(InLine), Parent(InParent)
		, ScopeLine(InScopeLine)
		{}
		
		SpvId GetNameId() const { return Name; }
		SpvId GetTypeDescId() const { return TypeDesc; }
		SpvId GetLine() const { return Line; }
		SpvId GetParentId() const { return Parent; }
		SpvId GetScopeLine() const { return ScopeLine; }
		
	private:
		SpvId Name;
		SpvId TypeDesc;
		SpvId Line;
		SpvId Parent;
		SpvId ScopeLine;
	};

	class SpvDebugScope : public SpvInstructionBase<SpvDebugScope>
	{
	public:
		SpvDebugScope(SpvId InScope) : SpvInstructionBase(SpvDebugInfo100::DebugScope)
		, Scope(InScope)
		{}
		
		SpvId GetScope() const { return Scope; }
		
	private:
		SpvId Scope;
	};

	class SpvDebugLine : public SpvInstructionBase<SpvDebugLine>
	{
	public:
		SpvDebugLine(SpvId InLineStart) : SpvInstructionBase(SpvDebugInfo100::DebugLine)
		, LineStart(InLineStart)
		{}
		
		SpvId GetLineStart() const { return LineStart; }
		
	private:
		SpvId LineStart;
	};

	class SpvDebugDeclare : public SpvInstructionBase<SpvDebugDeclare>
	{
	public:
		SpvDebugDeclare(SpvId InVarDesc, SpvId InPointerId) : SpvInstructionBase(SpvDebugInfo100::DebugDeclare)
		, VarDesc(InVarDesc)
		, PointerId(InPointerId)
		{}
		
		SpvId GetVarDesc() const { return VarDesc; }
		SpvId GetPointer() const { return PointerId; }
		
	private:
		SpvId VarDesc;
		SpvId PointerId;
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

	class SpvFAbs : public SpvInstructionBase<SpvFAbs>
	{
	public:
		SpvFAbs(SpvId InResultType, SpvId InX) : SpvInstructionBase(SpvGLSLstd450::FAbs)
		, ResultType(InResultType), X(InX)
		{}
		
		SpvId GetResultType() const { return ResultType; }
		SpvId GetX() const { return X; }
		
	private:
		SpvId ResultType;
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
