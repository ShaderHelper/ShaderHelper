#pragma once
#include "GpuResourceCommon.h"
#include <Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"

DECLARE_LOG_CATEGORY_EXTERN(LogShader, Log, All);
inline DEFINE_LOG_CATEGORY(LogShader);

namespace HLSL
{
    enum class CandidateKind
    {
        Unknown,
        KeyWord,
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
		Preprocess,
		Comment,
		String,
		
		//Parser
		Func,
		Type,
		Parm,
		Var,
		LocalVar,
		
		Other,
		
	};

    inline const TCHAR* Punctuations[] = {
        TEXT(":"), TEXT("+="), TEXT("++"), TEXT("+"), TEXT("--"), TEXT("-="), TEXT("-"), TEXT("("),
		TEXT(")"), TEXT("["), TEXT("]"), TEXT("."), TEXT("->"), TEXT("!="), TEXT("!"),
		TEXT("&="), TEXT("~"), TEXT("&"), TEXT("*="), TEXT("*"), TEXT("->"), TEXT("/="),
		TEXT("/"), TEXT("%="), TEXT("%"), TEXT("<<="), TEXT("<<"), TEXT("<="),
		TEXT("<"), TEXT(">>="), TEXT(">>"), TEXT(">="), TEXT(">"), TEXT("=="), TEXT("&&"),
		TEXT("&"), TEXT("^="), TEXT("^"), TEXT("|="), TEXT("||"), TEXT("|"), TEXT("?"),
		TEXT("="), TEXT(","), TEXT(";"), TEXT("{"), TEXT("}")
    };

    extern FRAMEWORK_API TArray<FString> BuiltinTypes;
    extern FRAMEWORK_API TArray<FString> KeyWords;
    extern FRAMEWORK_API TArray<FString> BuiltinFuncs;
}

namespace FW
{
	enum class GpuShaderFlag
	{
		Enable16bitType,
	};

	struct GpuShaderModel
	{
		uint8 Major = 6;
		uint8 Minor = 0;
	};

	class GpuShaderPreProcessor
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

    class GpuShader : public GpuResource
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

		void AddFlag(GpuShaderFlag InFlag) { Flags.Add(InFlag); }
		void AddFlags(TArray<GpuShaderFlag> InFlags) { Flags.Append(MoveTemp(InFlags)); }
		bool HasFlag(GpuShaderFlag InFlag) const { return Flags.Find(InFlag) != INDEX_NONE; }
        
    protected:
        ShaderType Type;
        FString ShaderName;
        FString EntryPoint;
        //Hlsl
        FString SourceText;
		FString ProcessedSourceText;
        
        TOptional<FString> FileName;
        TArray<FString> IncludeDirs;

		TArray<GpuShaderFlag> Flags;
    };

	struct ShaderErrorInfo
	{
		int32 Row, Col;
		FString Info;
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

	FRAMEWORK_API FString AdjustErrorLineNumber(const FString& ErrorInfo, int32 Delta);
    FRAMEWORK_API TArray<ShaderCandidateInfo> DefaultCandidates();

    class FRAMEWORK_API ShaderTU
    {
    public:
		ShaderTU(FStringView HlslSource, const TArray<FString>& IncludeDirs = {});
        TArray<ShaderErrorInfo> GetDiagnostic();
		HLSL::TokenType GetTokenType(HLSL::TokenType InType, uint32 Row, uint32 Col);
        TArray<ShaderCandidateInfo> GetCodeComplete(uint32 Row, uint32 Col);
        
    private:
        TPimplPtr<struct ShaderTUImpl> Impl;
    };

}
