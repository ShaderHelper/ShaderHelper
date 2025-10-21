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
		Matrix,
		Composite,
		Member,
		Function,
		Array,
	};

	class SpvTypeDesc
	{
	public:
		SpvTypeDesc(SpvTypeDescKind InKind) : Kind(InKind){}
		virtual ~SpvTypeDesc() = default;

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

	class SpvMatrixTypeDesc : public SpvTypeDesc
	{
	public:
		SpvMatrixTypeDesc(SpvVectorTypeDesc* InVectorTypeDesc, int32 InVectorCount) : SpvTypeDesc(SpvTypeDescKind::Matrix)
		, VectorTypeDesc(InVectorTypeDesc), VectorCount(InVectorCount)
		{}
		
		SpvVectorTypeDesc* GetVectorTypeDesc() const { return VectorTypeDesc; }
		int32 GetVectorCount() const { return VectorCount; }
		
	private:
		SpvVectorTypeDesc* VectorTypeDesc;
		int32 VectorCount;
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
		SpvArrayTypeDesc(SpvTypeDesc* InBaseTypeDesc, const TArray<int32>& InCompCounts) : SpvTypeDesc(SpvTypeDescKind::Array)
		, BaseTypeDesc(InBaseTypeDesc), CompCounts(InCompCounts)
		{}
		
		SpvTypeDesc* GetBaseTypeDesc() const { return BaseTypeDesc; }
		const TArray<int32>& GetCompCounts() const { return CompCounts; }
		int32 GetElementNum() const {
			int ElementNum = 1;
			for(int32 CompCount : CompCounts)
			{
				ElementNum *= CompCount;
			}
			return ElementNum;
		}
		
	private:
		SpvTypeDesc* BaseTypeDesc;
		TArray<int32> CompCounts;
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
		virtual ~SpvLexicalScope() = default;
		
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
		SpvFunctionDesc(SpvLexicalScope* InParent, const FString& InName, SpvFuncTypeDesc* InTypeDesc, int32 InLine, int32 InScopeLine)
		: SpvLexicalScope(SpvScopeKind::Function, InParent)
		, Name(InName)
		, TypeDesc(InTypeDesc)
		, Line(InLine)
		, ScopeLine(InScopeLine)
		{}
		
		int32 GetLine() const override { return Line; }
		int32 GetScopeLine() const { return ScopeLine; }
		FString GetName() const { return Name; }
		SpvFuncTypeDesc* GetFuncTypeDesc() const { return TypeDesc; }
		
	private:
		FString Name;
		SpvFuncTypeDesc* TypeDesc;
		int32 Line;
		int32 ScopeLine;
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
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Array)
		{
			SpvArrayTypeDesc* ArrayTypeDesc = static_cast<SpvArrayTypeDesc*>(TypeDesc);
			FString TypeName = GetTypeDescStr(ArrayTypeDesc->GetBaseTypeDesc());
			for(int32 CompCount : ArrayTypeDesc->GetCompCounts())
			{
				TypeName += FString::Printf(TEXT("[%d]"), CompCount);
			}
			return TypeName;
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Matrix)
		{
			SpvMatrixTypeDesc* MatrixTypeDesc = static_cast<SpvMatrixTypeDesc*>(TypeDesc);
			int32 VectorCount = MatrixTypeDesc->GetVectorCount();
			FString BasicTypeName = GetTypeDescStr(MatrixTypeDesc->GetVectorTypeDesc()->GetBasicTypeDesc());
			FString TypeName = BasicTypeName + FString::Printf(TEXT("%dx%d"), VectorCount, MatrixTypeDesc->GetVectorTypeDesc()->GetCompCount());
			return TypeName;
		}
		AUX::Unreachable();
	};

	inline FString GetFunctionSig(SpvFunctionDesc* FuncDesc, const TArray<ShaderFunc>& Funcs)
	{
		const ShaderFunc* EditorFunc = Funcs.FindByPredicate([&](const ShaderFunc& InItem) {
			return InItem.FullName == FuncDesc->GetName() && InItem.Start.x == FuncDesc->GetLine();
		});

		FString FuncName = FuncDesc->GetName();
		FuncName = FuncName.Replace(TEXT("."), TEXT("::"));
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
			FString SemaStr;
			if (EditorFunc)
			{
				const ShaderParameter& EditorParameter = EditorFunc->Params[i];
				if (EditorParameter.Name == "this")
				{
					continue;
				}

				if(EditorParameter.SemaFlag == ParamSemaFlag::In)
				{
					SemaStr = "in";
				}
				else if(EditorParameter.SemaFlag == ParamSemaFlag::Out)
				{
					SemaStr = "out";
				}
				else if (EditorParameter.SemaFlag == ParamSemaFlag::Inout)
				{
					SemaStr = "inout";
				}
			}
			if (!SemaStr.IsEmpty())
			{
				Signature += SemaStr + " ";
			}
			
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
		bool bGlobal{};
	};

	inline int32 GetTypeByteSize(const SpvTypeDesc* TypeDesc)
	{
		if(TypeDesc->GetKind() == SpvTypeDescKind::Basic)
		{
			return static_cast<const SpvBasicTypeDesc*>(TypeDesc)->GetSize() / 8;
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Vector)
		{
			const SpvVectorTypeDesc* VectorTypeDesc = static_cast<const SpvVectorTypeDesc*>(TypeDesc);
			int32 BasicTypeSize = GetTypeByteSize(VectorTypeDesc->GetBasicTypeDesc());
			return BasicTypeSize * VectorTypeDesc->GetCompCount();
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Composite)
		{
			const SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<const SpvCompositeTypeDesc*>(TypeDesc);
			const TArray<SpvTypeDesc*>& MemberTypeDescs = CompositeTypeDesc->GetMemberTypeDescs();
			int32 CompositeTypeSize{};
			for(int Index = 0; Index < MemberTypeDescs.Num(); Index++)
			{
				if(MemberTypeDescs[Index]->GetKind() == SpvTypeDescKind::Member)
				{
					SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(MemberTypeDescs[Index]);
					CompositeTypeSize += GetTypeByteSize(MemberTypeDesc);
				}
			}
			return CompositeTypeSize;
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Member)
		{
			return static_cast<const SpvMemberTypeDesc*>(TypeDesc)->GetSize() / 8;
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Array)
		{
			const SpvArrayTypeDesc* ArrayTypeDesc = static_cast<const SpvArrayTypeDesc*>(TypeDesc);
			return GetTypeByteSize(ArrayTypeDesc->GetBaseTypeDesc()) * ArrayTypeDesc->GetElementNum();
		}
		AUX::Unreachable();
	}

	inline FString GetValueStr(TArrayView<const uint8> InValue, const SpvTypeDesc* TypeDesc, const TArray<Vector2i>& InitializedRanges, int32 InOffset)
	{
		FString ValueStr;
		int32 Offset = InOffset;
		if(TypeDesc->GetKind() == SpvTypeDescKind::Vector)
		{
			ValueStr += "{";
			const SpvVectorTypeDesc* VectorTypeDesc = static_cast<const SpvVectorTypeDesc*>(TypeDesc);
			int32 BasicTypeSize = GetTypeByteSize(VectorTypeDesc->GetBasicTypeDesc());
			for(int Index = 0; Index < VectorTypeDesc->GetCompCount(); Index++)
			{
				ValueStr += GetValueStr(InValue, VectorTypeDesc->GetBasicTypeDesc(), InitializedRanges, Offset);
				Offset += BasicTypeSize;
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
			FString BasicValueStr = LOCALIZATION("Uninitialized").ToString();
			for(const auto& Range : InitializedRanges)
			{
				if(Offset >= Range.X && Offset + GetTypeByteSize(BasicTypeDesc) <= Range.Y)
				{
					if(Encoding == SpvDebugBasicTypeEncoding::Float)
					{
						float Value = *(float*)(InValue.GetData() + Offset);
						BasicValueStr = FString::Printf(TEXT("%.7g"), Value);
					}
					else if(Encoding == SpvDebugBasicTypeEncoding::Signed)
					{
						int Value = *(int*)(InValue.GetData() + Offset);
						BasicValueStr = FString::Format(TEXT("{0}"), {Value});
					}
					else if(Encoding == SpvDebugBasicTypeEncoding::Unsigned || Encoding == SpvDebugBasicTypeEncoding::Boolean)
					{
						uint32 Value = *(uint32*)(InValue.GetData() + Offset);
						BasicValueStr = FString::Format(TEXT("{0}"), {Value});
					}
					break;
				}
			}
			ValueStr += BasicValueStr;
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Composite)
		{
			const SpvCompositeTypeDesc* CompositeTypeDesc = static_cast<const SpvCompositeTypeDesc*>(TypeDesc);
			const TArray<SpvTypeDesc*>& MemberTypeDescs = CompositeTypeDesc->GetMemberTypeDescs();
			ValueStr += "{";
			for(int Index = 0; Index < MemberTypeDescs.Num(); Index++)
			{
				if(MemberTypeDescs[Index]->GetKind() == SpvTypeDescKind::Member)
				{
					SpvMemberTypeDesc* MemberTypeDesc = static_cast<SpvMemberTypeDesc*>(MemberTypeDescs[Index]);
					int32 MemberSize = GetTypeByteSize(MemberTypeDesc);
					ValueStr += MemberTypeDesc->GetName() + "=";
					ValueStr += GetValueStr(InValue, MemberTypeDesc->GetTypeDesc(), InitializedRanges, Offset);
					Offset += MemberSize;
				}
				
				if(Index != MemberTypeDescs.Num() - 1)
				{
					ValueStr += ", ";
				}
			}
			ValueStr += "}";
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Array)
		{
			const SpvArrayTypeDesc* ArrayTypeDesc = static_cast<const SpvArrayTypeDesc*>(TypeDesc);
			SpvTypeDesc* BaseTypeDesc = ArrayTypeDesc->GetBaseTypeDesc();
			int32 BaseTypeSize = GetTypeByteSize(BaseTypeDesc);
			int32 CompCount = ArrayTypeDesc->GetCompCounts()[0];
			SpvTypeDesc* ElementTypeDesc = BaseTypeDesc;
			int32 ElementTypeSize = BaseTypeSize;
			TUniquePtr<SpvArrayTypeDesc> SubArrayTypeDesc = ArrayTypeDesc->GetCompCounts().Num() > 1 ? MakeUnique<SpvArrayTypeDesc>(BaseTypeDesc, TArray{ArrayTypeDesc->GetCompCounts().GetData() + 1, ArrayTypeDesc->GetCompCounts().Num() -1 }) : nullptr;
			if(SubArrayTypeDesc)
			{
				ElementTypeDesc = SubArrayTypeDesc.Get();
				ElementTypeSize *= SubArrayTypeDesc->GetElementNum();
			}
			ValueStr += "[";
			for(int32 Index = 0; Index < CompCount; Index++)
			{
				ValueStr += GetValueStr(InValue, ElementTypeDesc, InitializedRanges, Offset);
				Offset += ElementTypeSize;
				
				if(Index != CompCount - 1)
				{
					ValueStr += ", ";
				}
			}
			ValueStr += "]";
		}
		else if(TypeDesc->GetKind() == SpvTypeDescKind::Matrix)
		{
			const SpvMatrixTypeDesc* MatrixTypeDesc = static_cast<const SpvMatrixTypeDesc*>(TypeDesc);
			SpvVectorTypeDesc* ElementTypeDesc = MatrixTypeDesc->GetVectorTypeDesc();
			int32 VectorCount = MatrixTypeDesc->GetVectorCount();
			int32 ElementTypeSize = GetTypeByteSize(ElementTypeDesc);
			ValueStr += "[";
			for(int32 Index = 0 ; Index < VectorCount; Index++)
			{
				ValueStr += GetValueStr(InValue, ElementTypeDesc, InitializedRanges, Offset);
				Offset += ElementTypeSize;
				
				if(Index != VectorCount - 1)
				{
					ValueStr += ", ";
				}
			}
			ValueStr += "]";
		}
		else
		{
			AUX::Unreachable();
		}
		return ValueStr;
	}

}
