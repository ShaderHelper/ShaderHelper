#pragma once

namespace FW
{

	enum class SpvExtSet
	{
		GLSLstd450,
		NonSemanticShaderDebugInfo100,
	};

	enum class SpvGLSLstd450
	{
		Round = 1,
		RoundEven = 2,
		Trunc = 3,
		FAbs = 4,
		SAbs = 5,
		FSign = 6,
		SSign = 7,
		Floor = 8,
		Ceil = 9,
		Fract = 10,
		Radians = 11,
		Degrees = 12,
		Sin = 13,
		Cos = 14,
		Tan = 15,
		Asin = 16,
		Acos = 17,
		Atan = 18,
		Sinh = 19,
		Cosh = 20,
		Tanh = 21,
		Asinh = 22,
		Acosh = 23,
		Atanh = 24,
		Atan2 = 25,
		Pow = 26,
		Exp = 27,
		Log = 28,
		Exp2 = 29,
		Log2 = 30,
		Sqrt = 31,
		InverseSqrt = 32,
		Determinant = 33,
		MatrixInverse = 34,
		Modf = 35,
		ModfStruct = 36,
		FMin = 37,
		UMin = 38,
		SMin = 39,
		FMax = 40,
		UMax = 41,
		SMax = 42,
		FClamp = 43,
		UClamp = 44,
		SClamp = 45,
		FMix = 46,
		IMix = 47,
		Step = 48,
		SmoothStep = 49,
		Fma = 50,
		Frexp = 51,
		FrexpStruct = 52,
		Ldexp = 53,
		PackSnorm4x8 = 54,
		PackUnorm4x8 = 55,
		PackSnorm2x16 = 56,
		PackUnorm2x16 = 57,
		PackHalf2x16 = 58,
		PackDouble2x32 = 59,
		UnpackSnorm2x16 = 60,
		UnpackUnorm2x16 = 61,
		UnpackHalf2x16 = 62,
		UnpackSnorm4x8 = 63,
		UnpackUnorm4x8 = 64,
		UnpackDouble2x32 = 65,
		Length = 66,
		Distance = 67,
		Cross = 68,
		Normalize = 69,
		FaceForward = 70,
		Reflect = 71,
		Refract = 72,
		FindILsb = 73,
		FindSMsb = 74,
		FindUMsb = 75,
		InterpolateAtCentroid = 76,
		InterpolateAtSample = 77,
		InterpolateAtOffset = 78,
		NMin = 79,
		NMax = 80,
		NClamp = 81,
	};

	enum class SpvDebugInfo100
	{
		DebugInfoNone = 0,
		DebugCompilationUnit = 1,
		DebugTypeBasic = 2,
		DebugTypePointer = 3,
		DebugTypeQualifier = 4,
		DebugTypeArray = 5,
		DebugTypeVector = 6,
		DebugTypedef = 7,
		DebugTypeFunction = 8,
		DebugTypeEnum = 9,
		DebugTypeComposite = 10,
		DebugTypeMember = 11,
		DebugTypeInheritance = 12,
		DebugTypePtrToMember = 13,
		DebugTypeTemplate = 14,
		DebugTypeTemplateParameter = 15,
		DebugTypeTemplateTemplateParameter = 16,
		DebugTypeTemplateParameterPack = 17,
		DebugGlobalVariable = 18,
		DebugFunctionDeclaration = 19,
		DebugFunction = 20,
		DebugLexicalBlock = 21,
		DebugLexicalBlockDiscriminator = 22,
		DebugScope = 23,
		DebugNoScope = 24,
		DebugInlinedAt = 25,
		DebugLocalVariable = 26,
		DebugInlinedVariable = 27,
		DebugDeclare = 28,
		DebugValue = 29,
		DebugOperation = 30,
		DebugExpression = 31,
		DebugMacroDef = 32,
		DebugMacroUndef = 33,
		DebugImportedEntity = 34,
		DebugSource = 35,
		DebugFunctionDefinition = 101,
		DebugSourceContinued = 102,
		DebugLine = 103,
		DebugNoLine = 104,
		DebugBuildIdentifier = 105,
		DebugStoragePath = 106,
		DebugEntryPoint = 107,
		DebugTypeMatrix = 108,
	};

	enum class SpvDebugBasicTypeEncoding
	{
		Unspecified = 0,
		Address = 1,
		Boolean = 2,
		Float = 3,
		Signed = 4,
		SignedChar = 5,
		Unsigned = 6,
		UnsignedChar = 7,
	};

	enum class SpvTypeDescKind
	{
		Basic,
		Vector,
		Composite,
		Member,
		Function,
		Array,
	};

	class SpvTypeDesc
	{
	public:
		SpvTypeDesc(SpvTypeDescKind InKind) : Kind(InKind){}
		SpvTypeDescKind GetKind() const { return Kind; }
	private:
		SpvTypeDescKind Kind;
	};

	class SpvBasicTypeDesc : public SpvTypeDesc
	{
	public:
		SpvBasicTypeDesc(const FString& InName, int32 InSize, SpvDebugBasicTypeEncoding InEncoding) : SpvTypeDesc(SpvTypeDescKind::Basic)
		, Name(InName), Size(InSize), Encoding(InEncoding)
		{}
		
		FString GetName() const { return Name; }
		//Bit size
		int32 GetSize() const { return Size; }
		SpvDebugBasicTypeEncoding GetEncoding() const { return Encoding; }
		
	private:
		FString Name;
		int32 Size;
		SpvDebugBasicTypeEncoding Encoding;
	};

	class SpvVectorTypeDesc : public SpvTypeDesc
	{
	public:
		SpvVectorTypeDesc(SpvBasicTypeDesc* InBasicTypeDesc, int32 InCompCount) : SpvTypeDesc(SpvTypeDescKind::Vector)
		, BasicTypeDesc(InBasicTypeDesc), CompCount(InCompCount)
		{}
		
		SpvBasicTypeDesc* GetBasicTypeDesc() const { return BasicTypeDesc; }
		int32 GetCompCount() const { return CompCount; }
		
	private:
		SpvBasicTypeDesc* BasicTypeDesc;
		int32 CompCount;
	};

	class SpvCompositeTypeDesc : public SpvTypeDesc
	{
	public:
		SpvCompositeTypeDesc(const FString& InName, int32 InSize, const TArray<SpvTypeDesc*>& InMemberTypeDescs) : SpvTypeDesc(SpvTypeDescKind::Composite)
		, Name(InName), Size(InSize), MemberTypeDescs(InMemberTypeDescs)
		{}
		
		FString GetName() const { return Name; }
		int32 GetSize() const { return Size; }
		const TArray<SpvTypeDesc*>& GetMemberTypeDescs() const { return MemberTypeDescs; }
		
	private:
		FString Name;
		int32 Size;
		TArray<SpvTypeDesc*> MemberTypeDescs;
	};

	class SpvMemberTypeDesc : public SpvTypeDesc
	{
	public:
		SpvMemberTypeDesc(const FString& InName, SpvTypeDesc* InTypeDesc, int32 InOffset, int32 InSize) : SpvTypeDesc(SpvTypeDescKind::Member)
		, Name(InName), TypeDesc(InTypeDesc), Offset(InOffset), Size(InSize)
		{}
		
		FString GetName() const { return Name; }
		SpvTypeDesc* GetTypeDesc() const { return TypeDesc; }
		//In Bits
		int32 GetOffset() const {return Offset; }
		int32 GetSize() const { return Size; }
		
	private:
		FString Name;
		SpvTypeDesc* TypeDesc;
		int32 Offset;
		int32 Size;
	};

	class SpvFuncTypeDesc : public SpvTypeDesc
	{
	public:
		SpvFuncTypeDesc(std::variant<SpvVoidType*, SpvTypeDesc*> InReturnType, const TArray<SpvTypeDesc*>& InParmTypes) : SpvTypeDesc(SpvTypeDescKind::Function)
		, ReturnType(InReturnType)
		, ParmTypes(InParmTypes)
		{}
		
		std::variant<SpvVoidType*, SpvTypeDesc*> GetReturnType() const { return ReturnType; }
		const TArray<SpvTypeDesc*>& GetParmTypes() const { return ParmTypes; }
		
	private:
		std::variant<SpvVoidType*, SpvTypeDesc*> ReturnType;
		TArray<SpvTypeDesc*> ParmTypes;
	};

	class SpvArrayTypeDesc : public SpvTypeDesc
	{
	public:
		SpvArrayTypeDesc(SpvTypeDesc* InBaseTypeDesc, const TArray<uint32>& InCompCounts) : SpvTypeDesc(SpvTypeDescKind::Array)
		, BaseTypeDesc(InBaseTypeDesc), CompCounts(InCompCounts)
		{}
		
		SpvTypeDesc* GetBaseTypeDesc() const { return BaseTypeDesc; }
		const TArray<uint32>& GetCompCounts() const { return CompCounts; }
		
	private:
		SpvTypeDesc* BaseTypeDesc;
		TArray<uint32> CompCounts;
	};
	
	enum class SpvScopeKind
	{
		TU,
		Function,
		Block,
	};

	class SpvLexicalScope
	{
	public:
		SpvLexicalScope(SpvScopeKind InKind, SpvLexicalScope* InParent)
		: Kind(InKind), Parent(InParent)
		{}
		
		virtual int32 GetLine() const {return 0;}
		SpvScopeKind GetKind() const { return Kind; }
		SpvLexicalScope* GetParent() const { return Parent; }
		bool Contains(SpvLexicalScope* Child) const {
			if(!Child)
			{
				return false;
			}
			else if(Child == this)
			{
				return true;
			}
			else
			{
				return Contains(Child->GetParent());
			}
		}
		
	private:
		SpvScopeKind Kind;
		SpvLexicalScope* Parent;
	};

	class SpvCompilationUnit :  public SpvLexicalScope
	{
	public:
		SpvCompilationUnit() : SpvLexicalScope(SpvScopeKind::TU, nullptr)
		{}
	};

	class SpvLexicalBlock : public SpvLexicalScope
	{
	public:
		SpvLexicalBlock(int32 InLine, SpvLexicalScope* InParent) : SpvLexicalScope(SpvScopeKind::Block, InParent)
		, Line(InLine)
		{}
		
		int32 GetLine() const override { return Line; }
		
	private:
		int32 Line;
	};

	class SpvFunctionDesc : public SpvLexicalScope
	{
	public:
		SpvFunctionDesc(SpvLexicalScope* InParent, const FString& InName, SpvFuncTypeDesc* InTypeDesc, int32 InLine)
		: SpvLexicalScope(SpvScopeKind::Function, InParent)
		, Name(InName)
		, TypeDesc(InTypeDesc)
		, Line(InLine)
		{}
		
		int32 GetLine() const override { return Line; }
		FString GetName() const { return Name; }
		SpvFuncTypeDesc* GetFuncTypeDesc() const { return TypeDesc; }
		
	private:
		FString Name;
		SpvFuncTypeDesc* TypeDesc;
		int32 Line;
	};

	inline SpvFunctionDesc* GetFunctionDesc(SpvLexicalScope* InScope)
	{
		if(!InScope || InScope->GetKind() == SpvScopeKind::TU)
		{
			return nullptr;
		}
		else if(InScope->GetKind() == SpvScopeKind::Function)
		{
			return static_cast<SpvFunctionDesc*>(InScope);
		}
		else
		{
			return GetFunctionDesc(InScope->GetParent());
		}
	};

	inline FString GetTypeDescStr(SpvTypeDesc* TypeDesc)
	{
		if(TypeDesc->GetKind() == SpvTypeDescKind::Vector)
		{
			SpvVectorTypeDesc* VectorTypeDesc = static_cast<SpvVectorTypeDesc*>(TypeDesc);
			if(VectorTypeDesc->GetCompCount() > 1)
			{
				return VectorTypeDesc->GetBasicTypeDesc()->GetName() + FString::FromInt(VectorTypeDesc->GetCompCount());
			}
			return VectorTypeDesc->GetBasicTypeDesc()->GetName();
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Basic)
		{
			SpvBasicTypeDesc* BasicTypeDesc = static_cast<SpvBasicTypeDesc*>(TypeDesc);
			return BasicTypeDesc->GetName();
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Composite)
		{
			SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<SpvCompositeTypeDesc*>(TypeDesc);
			return CompositeTypeDesc->GetName();
		}
		AUX::Unreachable();
	};

	inline FString GetFunctionSig(SpvFunctionDesc* FuncDesc)
	{
		FString FuncName = FuncDesc->GetName();
		SpvFuncTypeDesc* FuncTypeDesc = FuncDesc->GetFuncTypeDesc();
		FString Signature;
		auto ReturnType = FuncTypeDesc->GetReturnType();
		if(std::holds_alternative<SpvVoidType*>(ReturnType))
		{
			Signature += "void";
		}
		else
		{
			Signature += GetTypeDescStr(std::get<SpvTypeDesc*>(ReturnType));
		}
		Signature += " ";
		Signature += FuncName;
		Signature += "(";
		auto ParmTypes = FuncTypeDesc->GetParmTypes();
		for(int i = 0; i < ParmTypes.Num(); i++)
		{
			if(i == ParmTypes.Num() - 1)
			{
				Signature += GetTypeDescStr(ParmTypes[i]);
			}
			else
			{
				Signature += GetTypeDescStr(ParmTypes[i]);
				Signature += ", ";
			}
		}
		Signature += ")";
		return Signature;
	};

	struct SpvVariableDesc
	{
		FString Name;
		SpvTypeDesc* TypeDesc{};
		int32 Line;
		SpvLexicalScope* Parent{};
	};

	inline FString GetValueStr(const TArray<uint8>& InValue, const SpvTypeDesc* TypeDesc)
	{
		FString ValueStr;
		if(TypeDesc->GetKind() == SpvTypeDescKind::Vector)
		{
			ValueStr += "{";
			const SpvVectorTypeDesc* VectorTypeDesc = static_cast<const SpvVectorTypeDesc*>(TypeDesc);
			int32 BasicTypeSize = VectorTypeDesc->GetBasicTypeDesc()->GetSize() / 8;
			SpvDebugBasicTypeEncoding Encoding = VectorTypeDesc->GetBasicTypeDesc()->GetEncoding();
			for(int Index = 0; Index < VectorTypeDesc->GetCompCount(); Index++)
			{
				if(Encoding == SpvDebugBasicTypeEncoding::Float)
				{
					float Value = *(float*)(InValue.GetData() + BasicTypeSize * Index);
					ValueStr += FString::Format(TEXT("{0}"), {Value});
				}
				else if(Encoding == SpvDebugBasicTypeEncoding::Signed)
				{
					
				}
				else if(Encoding == SpvDebugBasicTypeEncoding::Unsigned)
				{
					
				}
				
				if(Index != VectorTypeDesc->GetCompCount() - 1)
				{
					ValueStr += ", ";
				}
			}
			ValueStr += "}";
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Basic)
		{
			const SpvBasicTypeDesc* BasicTypeDesc = static_cast<const SpvBasicTypeDesc*>(TypeDesc);
			SpvDebugBasicTypeEncoding Encoding = BasicTypeDesc->GetEncoding();
			if(Encoding == SpvDebugBasicTypeEncoding::Float)
			{
				float Value = *(float*)(InValue.GetData());
				ValueStr += FString::Format(TEXT("{0}"), {Value});
			}
			else if(Encoding == SpvDebugBasicTypeEncoding::Signed)
			{
				
			}
			else if(Encoding == SpvDebugBasicTypeEncoding::Unsigned)
			{
			}
		}
		return ValueStr;
	}

}
