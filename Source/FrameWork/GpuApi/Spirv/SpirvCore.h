#pragma once
#include "GpuApi/GpuResource.h"

namespace FW
{
	class SpvId
	{
	public:
		SpvId() : Value(0) {}
		SpvId(uint32 InValue) : Value(InValue) {}
		friend uint32 GetTypeHash(const SpvId& Key)
		{
			return ::GetTypeHash(Key.Value);
		}
		bool operator==(const SpvId&) const = default;
		uint32 GetValue() const { return Value; }
		
	private:
		uint32 Value;
	};

	enum class SpvTypeKind
	{
		Void,
		Bool,
		Float,
		Integer,
		Vector,
		Pointer,
		Struct,
		Image,
		Sampler,
		Array,
		RuntimeArray,
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
		ArrayStride = 6,
		BuiltIn = 11,
		Location = 30,
		Binding = 33,
		DescriptorSet = 34,
		Offset = 35,
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

	enum class SpvDim
	{
		_1D = 0,
		_2D = 1,
		_3D = 2,
		Cube = 3,
		Rect = 4,
		Buffer = 5,
		SubpassData = 6,
		TileImageDataEXT = 4173,
	};

	enum class SpvImageOperands : uint32
	{
		None = 0x0000,
		Bias = 0x0001,
		Lod = 0x0002,
		Grad = 0x0004,
		ConstOffset = 0x0008,
		Offset = 0x0010,
		ConstOffsets = 0x0020,
		Sample = 0x0040,
		MinLod = 0x0080,
		MakeTexelAvailable = 0x0100,
		MakeTexelVisible = 0x0200,
		NonPrivateTexel = 0x0400,
		VolatileTexel = 0x0800,
		SignExtend = 0x1000,
		ZeroExtend = 0x2000,
		Nontemporal = 0x4000,
		Offsets = 0x10000,
	};
	ENUM_CLASS_FLAGS(SpvImageOperands);

	enum class SpvImageFormat
	{
		Unknown = 0,
		Rgba32f = 1,
		Rgba16f = 2,
		R32f = 3,
		Rgba8 = 4,
		Rgba8Snorm = 5,
		Rg32f = 6,
		Rg16f = 7,
		R11fG11fB10f = 8,
		R16f = 9,
		Rgba16 = 10,
		Rgb10A2 = 11,
		Rg16 = 12,
		Rg8 = 13,
		R16 = 14,
		R8 = 15,
		Rgba16Snorm = 16,
		Rg16Snorm = 17,
		Rg8Snorm = 18,
		R16Snorm = 19,
		R8Snorm = 20,
		Rgba32i = 21,
		Rgba16i = 22,
		Rgba8i = 23,
		R32i = 24,
		Rg32i = 25,
		Rg16i = 26,
		Rg8i = 27,
		R16i = 28,
		R8i = 29,
		Rgba32ui = 30,
		Rgba16ui = 31,
		Rgba8ui = 32,
		R32ui = 33,
		Rgb10a2ui = 34,
		Rg32ui = 35,
		Rg16ui = 36,
		Rg8ui = 37,
		R16ui = 38,
		R8ui = 39,
		R64ui = 40,
		R64i = 41,
	};

	class SpvType
	{
	public:
		SpvTypeKind GetKind() const { return Kind; }
		SpvId GetId() const { return Id; }
		
		SpvType(SpvTypeKind InKind, SpvId InId) : Kind(InKind), Id(InId) {}
		virtual ~SpvType() = default;
		
		bool IsScalar() const { return Kind == SpvTypeKind::Bool || Kind == SpvTypeKind::Float
			|| Kind == SpvTypeKind::Integer; }
		bool IsComposite() const { return Kind == SpvTypeKind::Vector || Kind == SpvTypeKind::Struct || Kind == SpvTypeKind::Array; }
		
	protected:
		SpvTypeKind Kind;
		SpvId Id;
	};

	class SpvScalarType : public SpvType
	{
	public:
		SpvScalarType(SpvId InId, SpvTypeKind InKind, uint32 InWidth)
		: SpvType(InKind, InId)
		, BitWidth(InWidth)
		{}
		
		uint32 GetWidth() const { return BitWidth; }
		
	protected:
		uint32 BitWidth;
	};

	class SpvFloatType : public SpvScalarType
	{
	public:
		SpvFloatType(SpvId InId, uint32 InWidth) : SpvScalarType(InId, SpvTypeKind::Float, InWidth) {}
	};

	class SpvIntegerType : public SpvScalarType
	{
	public:
		SpvIntegerType(SpvId InId, uint32 InWidth, bool InIsSigned)
		: SpvScalarType(InId, SpvTypeKind::Integer, InWidth)
		, IsSigned(InIsSigned)
		{}
		
		bool IsSigend() const { return IsSigned; }
		
	protected:
		bool IsSigned;
	};

	class SpvImageType : public SpvType
	{
	public:
		SpvImageType(SpvId InId, SpvType* InSampledType, SpvDim InDim) : SpvType(SpvTypeKind::Image, InId)
		, SampledType(InSampledType), Dim(InDim)
		{}
		
		SpvType* SampledType;
		SpvDim Dim;
	};

	class SpvSamplerType : public SpvType
	{
	public:
		SpvSamplerType(SpvId InId) : SpvType(SpvTypeKind::Sampler, InId) {}
	};

	class SpvVoidType : public SpvType
	{
	public:
		SpvVoidType(SpvId InId) : SpvType(SpvTypeKind::Void, InId) {}
	};

	class SpvBoolType : public SpvScalarType
	{
	public:
		SpvBoolType(SpvId InId) : SpvScalarType(InId, SpvTypeKind::Bool, 32) {}
	};

	class SpvVectorType : public SpvType
	{
	public:
		SpvVectorType(SpvId InId, SpvScalarType* InElementType, uint32 InElementCount)
		: SpvType(SpvTypeKind::Vector, InId)
		, ElementType(InElementType)
		, ElementCount(InElementCount)
		{}

		SpvScalarType* ElementType;
		uint32 ElementCount;
	};

	class SpvPointerType : public SpvType
	{
	public:
		SpvPointerType(SpvId InId, SpvStorageClass InStorageClass, SpvType* InPointeeType)
		: SpvType(SpvTypeKind::Pointer, InId)
		, StorageClass(InStorageClass)
		, PointeeType(InPointeeType)
		{}
		
		SpvStorageClass StorageClass;
		SpvType* PointeeType;
	};

	class SpvStructType : public SpvType
	{
	public:
		SpvStructType(SpvId InId, const TArray<SpvType*>& InMemberTypes)
		: SpvType(SpvTypeKind::Struct, InId)
		, MemberTypes(InMemberTypes)
		{}

		TArray<SpvType*> MemberTypes;
	};

	class SpvArrayType : public SpvType
	{
	public:
		SpvArrayType(SpvId InId, SpvType* InElementType, uint32 InLength) : SpvType(SpvTypeKind::Array, InId)
		, ElementType(InElementType), Length(InLength)
		{}
	 
		SpvType* ElementType;
		uint32 Length;
	};

	class SpvRuntimeArrayType : public SpvType
	{
	public:
		SpvRuntimeArrayType(SpvId InId, SpvType* InElementType) : SpvType(SpvTypeKind::RuntimeArray, InId)
		, ElementType(InElementType)
		{}
		
		SpvType* ElementType;
	};

	inline FString GetHlslTypeStr(SpvType* Type)
	{
		if(Type->GetKind() == SpvTypeKind::Float)
		{
			return "float";
		}
		else if(Type->GetKind() == SpvTypeKind::Bool)
		{
			return "bool";
		}
		else if(Type->GetKind() == SpvTypeKind::Integer)
		{
			SpvIntegerType* IntegerType = static_cast<SpvIntegerType*>(Type);
			return IntegerType->IsSigend() ? "int" : "uint";
		}
		else if(Type->GetKind() == SpvTypeKind::Vector)
		{
			SpvVectorType* VectorType = static_cast<SpvVectorType*>(Type);
			FString TypeStr = GetHlslTypeStr(VectorType->ElementType);
			if(VectorType->ElementCount > 1)
			{
				TypeStr.AppendInt((int)VectorType->ElementCount);
			}
			return TypeStr;
		}
		AUX::Unreachable();
	};

	//Only valid for the type of the internal object,
	//because the type of the external object may have different memory alignment rules.
	inline int32 GetTypeByteSize(SpvType* Type)
	{
		if(Type->IsScalar())
		{
			SpvScalarType* ScalarType = static_cast<SpvScalarType*>(Type);
			return ScalarType->GetWidth() / 8;
		}
		else if(Type->GetKind() == SpvTypeKind::Vector)
		{
			SpvVectorType* VectorType = static_cast<SpvVectorType*>(Type);
			return GetTypeByteSize(VectorType->ElementType) * VectorType->ElementCount;
		}
		else if(Type->GetKind() == SpvTypeKind::Array)
		{
			SpvArrayType* ArrayType = static_cast<SpvArrayType*>(Type);
			return GetTypeByteSize(ArrayType->ElementType) * ArrayType->Length;
		}
		else if(Type->GetKind() == SpvTypeKind::Struct)
		{
			SpvStructType* StructType = static_cast<SpvStructType*>(Type);
			int32 MembersByteSize = 0;
			for(SpvType* MemberType : StructType->MemberTypes)
			{
				MembersByteSize += GetTypeByteSize(MemberType);
			}
			return MembersByteSize;
		}
		
		AUX::Unreachable();
	}

	struct SpvObject
	{
		SpvId Id{};
		SpvType* Type = nullptr;
		
		struct External
		{
			GpuResource* Resource = nullptr;
			TArray<uint8> Value;
			
			bool IsOpaque() const { return Value.IsEmpty() && Resource; }
		};
			
		struct Internal
		{
			TArray<uint8> Value;
			TArray<GpuResource*> Resources;
			
			bool IsOpaque() const { return Value.IsEmpty() && !Resources.IsEmpty(); }
		};
		
		bool IsOpaque() const { return std::visit([](auto&& Arg){ return Arg.IsOpaque(); }, Storage); }
		bool IsExternal() const { return std::holds_alternative<SpvObject::External>(Storage); }
		
		std::variant<External, Internal> Storage;
	};

	struct SpvVariable : SpvObject
	{
		SpvStorageClass StorageClass;
		TArray<Vector2i> InitializedRanges;//[Start,End]
	};

	struct SpvPointer
	{
		SpvId Id{};
		SpvVariable* Pointee;
		TArray<int32> Indexes;
	};

	struct SpvDecoration
	{
		SpvDecorationKind Kind;
		union
		{
			struct
			{
				SpvBuiltIn Attribute;
			} BuiltIn;
			struct
			{
				uint32 Number;
			}Binding, Location, DescriptorSet, ArrayStride;
			struct
			{
				uint32 MemberIndex;
				uint32 ByteOffset;
			}Offset;
		};
	};

	enum class SpvOp
	{
		String = 7,
		ExtInstImport = 11,
		ExtInst = 12,
		EntryPoint = 15,
		ExecutionMode = 16,
		TypeVoid = 19,
		TypeBool = 20,
		TypeInt = 21,
		TypeFloat = 22,
		TypeVector = 23,
		TypeImage = 25,
		TypeSampler = 26,
		TypeArray = 28,
		TypeRuntimeArray = 29,
		TypeStruct = 30,
		TypePointer = 32,
		ConstantTrue = 41,
		ConstantFalse = 42,
		Constant = 43,
		ConstantComposite = 44,
		ConstantNull = 46,
		Function = 54,
		FunctionParameter = 55,
		FunctionCall = 57,
		Variable = 59,
		Load = 61,
		Store = 62,
		AccessChain = 65,
		Decorate = 71,
		MemberDecorate = 72,
		VectorShuffle = 79,
		CompositeConstruct = 80,
		CompositeExtract = 81,
		SampledImage = 86,
		ImageSampleImplicitLod = 87,
		ConvertFToU = 109,
		ConvertFToS = 110,
		ConvertSToF = 111,
		ConvertUToF = 112,
		Bitcast = 124,
		FNegate = 127,
		IAdd = 128,
		FAdd = 129,
		ISub = 130,
		FSub = 131,
		IMul = 132,
		FMul = 133,
		UDiv = 134,
		SDiv = 135,
		FDiv = 136,
		UMod = 137,
		SRem = 138,
		FRem = 140,
		VectorTimesScalar = 142,
		Dot = 148,
		Any = 154,
		All = 155,
		IsNan = 156,
		IsInf = 157,
		LogicalOr = 166,
		LogicalAnd = 167,
		LogicalNot = 168,
		Select = 169,
		IEqual = 170,
		INotEqual = 171,
		SGreaterThan = 173,
		SLessThan = 177,
		FOrdLessThan = 184,
		FOrdGreaterThan = 186,
		ShiftRightLogical = 194,
		ShiftRightArithmetic = 195,
		ShiftLeftLogical = 196,
		BitwiseOr = 197,
		BitwiseXor = 198,
		BitwiseAnd = 199,
		Not = 200,
		DPdx = 207,
		DPdy = 208,
		Fwidth = 209,
		Label = 248,
		Branch = 249,
		BranchConditional = 250,
		Return = 253,
		ReturnValue = 254,
	};
}

template<>
struct std::hash<FW::SpvId> {
	std::size_t operator()(const FW::SpvId& Id) const
	{
		return std::hash<uint32>{}(Id.GetValue());
	}
};

