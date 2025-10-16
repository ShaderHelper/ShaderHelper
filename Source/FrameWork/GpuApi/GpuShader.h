#pragma once
#include "GpuResourceCommon.h"
#include <Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "GpuBindGroupLayout.h"

DECLARE_LOG_CATEGORY_EXTERN(LogShader, Log, All);
inline DEFINE_LOG_CATEGORY(LogShader);

namespace HLSL
{
    enum class CandidateKind
    {
        Unknown,
        Kword,
        Var,
        Func,
        Macro,
        Type
    };

	enum class TokenType
	{
		//Tokenizer
		Number,
		Keyword,
		Punctuation,
		BuildtinFunc,
		BuildtinType,
		Identifier,
		Comment,
		String,
		
		//Parser
		Func,
		Type,
		Parm,
		Var,
		LocalVar,

		Preprocess,
		Other,
		
	};

    inline const TCHAR* Punctuations[] = {
        TEXT(":"), TEXT("+="), TEXT("++"), TEXT("+"), TEXT("--"), TEXT("-="), TEXT("-"), TEXT("("),
		TEXT(")"), TEXT("["), TEXT("]"), TEXT("."), TEXT("->"), TEXT("!="), TEXT("!"),
		TEXT("&="), TEXT("~"), TEXT("&"), TEXT("*="), TEXT("*"), TEXT("->"), TEXT("/="),
		TEXT("/"), TEXT("%="), TEXT("%"), TEXT("<<="), TEXT("<<"), TEXT("<="),
		TEXT("<"), TEXT(">>="), TEXT(">>"), TEXT(">="), TEXT(">"), TEXT("=="), TEXT("&&"),
		TEXT("&"), TEXT("^="), TEXT("^"), TEXT("|="), TEXT("||"), TEXT("|"), TEXT("?"),
		TEXT("="), TEXT(","), TEXT(";"), TEXT("{"), TEXT("}"), TEXT("//"), TEXT("/*"), TEXT("*/")
    };

    extern FRAMEWORK_API TArray<FString> BuiltinTypes;
    extern FRAMEWORK_API TArray<FString> KeyWords;
    extern FRAMEWORK_API TArray<FString> BuiltinFuncs;
}

namespace FW
{
	enum class GpuShaderCompilerFlag : uint32
	{
		None = 0,
		Enable16bitType = 1u << 0,
		GenSpvForDebugging = 1u << 1,
	};
	ENUM_CLASS_FLAGS(GpuShaderCompilerFlag);

	struct GpuShaderModel
	{
		uint8 Major = 6;
		uint8 Minor = 0;
	};

	class FRAMEWORK_API GpuShaderPreProcessor
	{
	public:
		GpuShaderPreProcessor(FString InShaderText) : ShaderText(MoveTemp(InShaderText)) {}

	public:
		//hlsl does not currently support string literal
		GpuShaderPreProcessor& ReplacePrintStringLiteral();
		FString Finalize() { return MoveTemp(ShaderText); }

	private:
		FString ShaderText;
	};

    struct GpuShaderFileDesc
    {
        FString FileName;
        ShaderType Type;
        FString EntryPoint;
        FString ExtraDecl;
        TArray<FString> IncludeDirs = { 
            PathHelper::ShaderDir(), 
            FPaths::GetPath(*FileName)
        };
    };

    struct GpuShaderSourceDesc
    {
        FString Name;
        FString Source;
        ShaderType Type;
        FString EntryPoint;
        TArray<FString> IncludeDirs = { 
            PathHelper::ShaderDir() 
        };
    };

	struct GpuShaderLayoutBinding
	{
		FString Name;
		BindingSlot Slot;
		BindingGroupSlot Group;
		BindingType Type;
	};

    class FRAMEWORK_API GpuShader : public GpuResource
    {
	public:
        //From file
		GpuShader(const GpuShaderFileDesc& FileDesc);
    
        //From source
		GpuShader(const GpuShaderSourceDesc& SourceDesc);
        
        //Successfully got the bytecode result and no error occurred.
        virtual bool IsCompiled() const {
            return false;
        }
        
        TOptional<FString> GetFileName() const { return FileName; }
        const TArray<FString>& GetIncludeDirs() const { return IncludeDirs; }
        
        const FString& GetShaderName() const { return ShaderName; }
        ShaderType GetShaderType() const {return Type;}
        const FString& GetSourceText() const { return SourceText; }
		const FString& GetProcessedSourceText() const { return ProcessedSourceText; }
        const FString& GetEntryPoint() const { return EntryPoint; }
		GpuShaderModel GetShaderModelVer() const;

		virtual TArray<GpuShaderLayoutBinding> GetLayout() const = 0;

	public:
		GpuShaderCompilerFlag CompilerFlag = GpuShaderCompilerFlag::None;
		TArray<uint32> SpvCode;
        
    protected:
        ShaderType Type;
        FString ShaderName;
        FString EntryPoint;
        //Hlsl
        FString SourceText;
		FString ProcessedSourceText;
        
        TOptional<FString> FileName;
        TArray<FString> IncludeDirs;
    };

	struct ShaderDiagnosticInfo
	{
		uint32 Row, Col;
		FString Error;
		FString Warn;
	};

    struct ShaderCandidateInfo
    {
        HLSL::CandidateKind Kind;
        FString Text;
        
        bool operator==(const ShaderCandidateInfo& Other) const
        {
            return Kind == Other.Kind && Text == Other.Text;
        }
    };

	enum class ParamSemaFlag
	{
		None = 0,
		In = 1,
		Out = 2,
		Inout = 3
	};

	struct ShaderParameter
	{
		FString Name;
		ParamSemaFlag SemaFlag;
	};

	struct ShaderFunc
	{
		FString Name;
		FString FullName;
		Vector2i Start;
		Vector2i End;
		TArray<ShaderParameter> Params;
	};

	struct ShaderOccurrence
	{
		uint32 Row, Col;
	};

	struct ShaderScope
	{
		Vector2i Start;
		Vector2i End;
	};

	FRAMEWORK_API FString AdjustDiagLineNumber(const FString& ErrorInfo, int32 Delta);
    FRAMEWORK_API TArray<ShaderCandidateInfo> DefaultCandidates();

    class FRAMEWORK_API ShaderTU
    {
    public:
		ShaderTU();
		ShaderTU(FStringView HlslSource, const TArray<FString>& IncludeDirs = {});
		TArray<ShaderDiagnosticInfo> GetDiagnostic();
		HLSL::TokenType GetTokenType(HLSL::TokenType InType, uint32 Row, uint32 Col);
        TArray<ShaderCandidateInfo> GetCodeComplete(uint32 Row, uint32 Col);
		TArray<ShaderFunc> GetFuncs();
		TArray<ShaderOccurrence> GetOccurrences(uint32 Row, uint32 Col);
		TArray<ShaderScope> GetScopes();
        
    private:
        TPimplPtr<struct ShaderTUImpl> Impl;
    };

}
