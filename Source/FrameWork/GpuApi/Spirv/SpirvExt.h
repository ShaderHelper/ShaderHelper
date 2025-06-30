#pragma once

namespace FW
{

	enum class SpvExtSet
	{
		GLSLstd450,
		NonSemanticShaderDebugInfo100,
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
		
		virtual int32 GetLineNumber() const {return 0;}
		SpvScopeKind GetKind() const { return Kind; }
		SpvLexicalScope* GetParent() const { return Parent; }
		
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
		SpvLexicalBlock(int32 InLineNumber, SpvLexicalScope* InParent) : SpvLexicalScope(SpvScopeKind::Block, InParent)
		, LineNumber(InLineNumber)
		{}
		
		int32 GetLineNumber() const override { return LineNumber; }
		
	private:
		int32 LineNumber;
	};

	class SpvFunctionDesc : public SpvLexicalScope
	{
	public:
		SpvFunctionDesc(SpvLexicalScope* InParent, const FString& InName, SpvTypeDesc* InTypeDesc, int32 InLine)
		: SpvLexicalScope(SpvScopeKind::Function, InParent)
		, Name(InName)
		, TypeDesc(InTypeDesc)
		, LineNumber(InLine)
		{}
		
		int32 GetLineNumber() const override { return LineNumber; }
		FString GetName() const { return Name; }
		SpvTypeDesc* GetTypeDesc() const { return TypeDesc; }
		
	private:
		FString Name;
		SpvTypeDesc* TypeDesc;
		int32 LineNumber;
	};

	struct SpvVariableDesc
	{
		FString Name;
		SpvTypeDesc* TypeDesc{};
		int32 Line;
		SpvLexicalScope* Parent{};
	};

}
