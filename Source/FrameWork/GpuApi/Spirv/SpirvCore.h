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
		Array,
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
		Location = 30,
		Binding = 33,
		DescriptorSet = 34,
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
		
		bool IsScalar() const { return Kind == SpvTypeKind::Bool || Kind == SpvTypeKind::Float
			|| Kind == SpvTypeKind::Integer; }
		
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
		: SpvScalarType(SpvTypeKind::Integer, InWidth)
		, IsSigned(InIsSigned)
		{}
		
		bool IsSigend() const { return IsSigned; }
		
	private:
		bool IsSigned;
	};

	class SpvVoidType : public SpvType
	{
	public:
		SpvVoidType() : SpvType(SpvTypeKind::Void) {}
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

	class SpvArrayType : public SpvType
	{
	public:
		SpvArrayType(SpvType* InElementType, uint32 InLength) : SpvType(SpvTypeKind::Array)
		, ElementType(InElementType), Length(InLength)
		{}
	 
		SpvType* ElementType;
		uint32 Length;
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
			if(VectorType->ElementCount > 0)
			{
				TypeStr.AppendInt((int)VectorType->ElementCount);
			}
			return TypeStr;
		}
		AUX::Unreachable();
	};

	inline uint32 GetTypeByteSize(SpvType* Type)
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
			uint32 MembersByteSize = 0;
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
		};
			
		struct Internal
		{
			TArray<uint8> Value;
		};
		
		bool IsExternal() const { return std::holds_alternative<SpvObject::External>(Storage); }
		
		std::variant<External, Internal> Storage;
	};

	struct SpvVariable : SpvObject
	{
		bool Initialized{};
		SpvStorageClass StorageClass;
	};

	struct SpvPointer
	{
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
			}Binding;
			struct
			{
				uint32 Number;
			}Location;
			struct
			{
				uint32 Number;
			}DescriptorSet;
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
		TypeArray = 28,
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
		CompositeConstruct = 80,
		CompositeExtract = 81,
		FDiv = 136,
		IEqual = 170,
		INotEqual = 171,
		DPdx = 207,
		Label = 248,
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

