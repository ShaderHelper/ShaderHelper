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

	enum class SpvDebugBaseTypeEncoding
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

	struct SpvVariableDesc
	{
		
	};

	struct SpvTypeDesc
	{
		
	};
	
	enum class ScopeKind
	{
		TU,
		Function,
		Block,
	};

	struct SpvLexicalScope
	{
		ScopeKind Kind;
		SpvLexicalScope* Parent = nullptr;
	};

	struct SpvCompilationUnit : SpvLexicalScope
	{
		SpvCompilationUnit() : SpvLexicalScope{ScopeKind::TU}
		{}
	};

	struct SpvFunctionDesc : SpvLexicalScope
	{
		SpvFunctionDesc(SpvLexicalScope* InParent, const FString& InName, const SpvTypeDesc* InTypeDesc, uint32 InLine)
		: SpvLexicalScope{ScopeKind::Function, InParent}
		, Name(InName)
		, TypeDesc(InTypeDesc)
		, LineNumber(InLine)
		{}
		
		FString Name;
		const SpvTypeDesc* TypeDesc;
		uint32 LineNumber;
	};

}
