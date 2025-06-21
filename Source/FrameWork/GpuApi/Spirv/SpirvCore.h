#pragma once

namespace FW
{
	using SpvId = uint32;

	enum class SpvTypeKind
	{
		Bool,
		Float,
		Uint,
		Int,
		Vector,
		Pointer,
		Struct,
	};

	enum class SpvStorageClass
	{
		UniformConstant = 0,
		Input = 1,
		Uniform = 2,
		Output = 3,
		Workgroup = 4,
		CrossWorkgroup = 5,
		Private = 6,
		Function = 7,
		Generic = 8,
		PushConstant = 9,
		AtomicCounter = 10,
		Image = 11,
		StorageBuffer = 12,
	};

	enum class SpvDecorationKind
	{
		BuiltIn = 11,
	};

	enum class SpvBuiltIn
	{
		FragCoord = 15,
	};

	enum class SpvExecutionModel
	{
		Vertex = 0,
		TessellationControl = 1,
		TessellationEvaluation = 2,
		Geometry = 3,
		Fragment = 4,
		GLCompute = 5,
		Kernel = 6,
	};

	enum class SpvExecutionMode
	{
		LocalSize = 17,
	};

	class SpvType
	{
	public:
		SpvTypeKind GetKind() const { return Kind; }
		
		SpvType(SpvTypeKind InKind) : Kind(InKind) {}
		virtual ~SpvType() = default;
		
		bool IsScalar() const { return Kind != SpvTypeKind::Vector; }
		
	private:
		SpvTypeKind Kind;
	};

	class SpvScalarType : public SpvType
	{
	public:
		SpvScalarType(SpvTypeKind InKind, uint32 InWidth)
		: SpvType(InKind)
		, BitWidth(InWidth)
		{}
		
		uint32 GetWidth() const { return BitWidth; }
		
	private:
		uint32 BitWidth;
	};

	class SpvFloatType : public SpvScalarType
	{
	public:
		SpvFloatType(uint32 InWidth) : SpvScalarType(SpvTypeKind::Float, InWidth) {}
	};

	class SpvIntegerType : public SpvScalarType
	{
	public:
		SpvIntegerType(uint32 InWidth, bool InIsSigned)
		: SpvScalarType(IsSigned ? SpvTypeKind::Int : SpvTypeKind::Uint, InWidth)
		, IsSigned(InIsSigned)
		{}
		
		bool IsSigend() const { return IsSigned; }
		
	private:
		bool IsSigned;
	};

	class SpvBoolType : public SpvScalarType
	{
	public:
		SpvBoolType() : SpvScalarType(SpvTypeKind::Bool, 32) {}
	};

	class SpvVectorType : public SpvType
	{
	public:
		SpvVectorType(SpvScalarType* InElementType, uint32 InElementCount)
		: SpvType(SpvTypeKind::Vector)
		, ElementType(InElementType)
		, ElementCount(InElementCount)
		{}

		SpvScalarType* ElementType;
		uint32 ElementCount;
	};

	class SpvPointerType : public SpvType
	{
	public:
		SpvPointerType(SpvStorageClass InStorageClass, SpvType* InPointeeType)
		: SpvType(SpvTypeKind::Pointer)
		, StorageClass(InStorageClass)
		, PointeeType(InPointeeType)
		{}
		
		SpvStorageClass StorageClass;
		SpvType* PointeeType;
	};

	class SpvStructType : public SpvType
	{
	public:
		SpvStructType(const TArray<SpvType*>& InMemberTypes)
		: SpvType(SpvTypeKind::Struct)
		, MemberTypes(InMemberTypes)
		{}

		TArray<SpvType*> MemberTypes;
	};

	struct SpvObject
	{
		SpvType* Type = nullptr;
		TArray<uint8> Value;
	};

	struct SpvVariable : SpvObject
	{
		bool Initialized{};
	};

	struct SpvFunction
	{
		
	};

	struct SpvDecoration
	{
		SpvDecorationKind Kind;
	};

	struct SpvBuiltInDecoration : SpvDecoration
	{
		SpvBuiltInDecoration(SpvBuiltIn InBuiltIn)
		: SpvDecoration{SpvDecorationKind::BuiltIn}
		, BuiltIn(InBuiltIn)
		{}
		
		SpvBuiltIn BuiltIn;
	};

	enum class SpvOp
	{
		String = 7,
		ExtInstImport = 11,
		ExtInst = 12,
		EntryPoint = 15,
		ExecutionMode = 16,
		TypeBool = 20,
		TypeInt = 21,
		TypeFloat = 22,
		TypeVector = 23,
		TypeStruct = 30,
		TypePointer = 32,
		ConstantTrue = 41,
		ConstantFalse = 42,
		Constant = 43,
		ConstantComposite = 44,
		Function = 54,
		Decorate = 71,
	};
}
