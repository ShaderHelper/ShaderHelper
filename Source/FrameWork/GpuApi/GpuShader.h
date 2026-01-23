#pragma once
#include "GpuResourceCommon.h"
#include <Misc/FileHelper.h>
#include "Common/Path/PathHelper.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "GpuBindGroupLayout.h"
#include "ShaderConductor.hpp"

DECLARE_LOG_CATEGORY_EXTERN(LogShader, Log, All);
inline DEFINE_LOG_CATEGORY(LogShader);

namespace FW
{

	inline ShaderConductor::ShaderStage MapShaderCunductorStage(ShaderType InType)
	{
		switch (InType)
		{
		case ShaderType::VertexShader:         return ShaderConductor::ShaderStage::VertexShader;
		case ShaderType::PixelShader:          return ShaderConductor::ShaderStage::PixelShader;
		case ShaderType::ComputeShader:        return ShaderConductor::ShaderStage::ComputeShader;
		default:
			AUX::Unreachable();
		}
	}

	inline const TCHAR* Punctuations[] = {
		TEXT(":"), TEXT("+="), TEXT("++"), TEXT("+"), TEXT("--"), TEXT("-="), TEXT("-"), TEXT("("),
		TEXT(")"), TEXT("["), TEXT("]"), TEXT("."), TEXT("->"), TEXT("!="), TEXT("!"),
		TEXT("&="), TEXT("~"), TEXT("&"), TEXT("*="), TEXT("*"), TEXT("->"), TEXT("/="),
		TEXT("/"), TEXT("%="), TEXT("%"), TEXT("<<="), TEXT("<<"), TEXT("<="),
		TEXT("<"), TEXT(">>="), TEXT(">>"), TEXT(">="), TEXT(">"), TEXT("=="), TEXT("&&"),
		TEXT("&"), TEXT("^="), TEXT("^"), TEXT("|="), TEXT("||"), TEXT("|"), TEXT("?"),
		TEXT("="), TEXT(","), TEXT(";"), TEXT("{"), TEXT("}"), TEXT("//"), TEXT("/*"), TEXT("*/")
	};

	enum class ShaderCandidateKind
	{
		Unknown,
		Kword,
		Var,
		Func,
		Macro,
		Type
	};

	enum class ShaderTokenType
	{
		Unknown,
		//Tokenizer
		Number,
		Punctuation,
		Identifier,
		Comment,
		String,

		//Parser
		Keyword,
		BuiltinFunc,
		BuiltinType,
		Func,
		Type,
		Param,
		Var,
		LocalVar,

		Preprocess,
	};

	enum class GpuShaderLanguage
	{
		HLSL,
		GLSL
	};

	enum class GpuShaderCompilerFlag : uint32
	{
		None = 0,
		Enable16bitType = 1u << 0,
		GenSpvForDebugging = 1u << 1,
		CompileFromSpriv = 1u << 2,
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
		GpuShaderPreProcessor(FString InShaderText, GpuShaderLanguage InLanguage) : ShaderText(MoveTemp(InShaderText)), Language(InLanguage)
		{}

	public:
		//hlsl does not currently support string literal
		// For GLSL:
		// - default (false): replace string literal into `EXPAND(uint StrArr[] = uint[](...))` and rename Print/PrintAtMouse/Assert to Print%d...
		// - true: only rename Print/PrintAtMouse/Assert to Print%d..., keep the original string literal (do NOT generate uint StrArr array)
		GpuShaderPreProcessor& ReplacePrintStringLiteral(bool bGlslOnlyRenamePrintFuncs = false);
		FString Finalize() { return MoveTemp(ShaderText); }

	private:
		FString ShaderText;
		GpuShaderLanguage Language;
	};

    struct GpuShaderFileDesc
    {
        FString FileName;
        ShaderType Type;
        FString EntryPoint;
        FString ExtraDecl;
		GpuShaderLanguage Language = GpuShaderLanguage::HLSL;
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
		FString EntryPoint; //For HLSL compilation; For GLSL, always "main"
		GpuShaderLanguage Language = GpuShaderLanguage::HLSL;
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
		friend GpuShaderPreProcessor;
	public:
        //From file
		GpuShader(const GpuShaderFileDesc& FileDesc);
    
        //From source
		GpuShader(const GpuShaderSourceDesc& SourceDesc);
        
        //Successfully got the bytecode result and no error occurred.
        virtual bool IsCompiled() const {
            return false;
        }
        
		const FString& GetFileName() const { return FileName; }
        const TArray<FString>& GetIncludeDirs() const { return IncludeDirs; }
        
        const FString& GetShaderName() const { return ShaderName; }
        ShaderType GetShaderType() const {return Type;}
        const FString& GetSourceText() const { return SourceText; }
		const FString& GetProcessedSourceText() const { return ProcessedSourceText; }
		FString GetEntryPoint() const {
			return ShaderLanguage == GpuShaderLanguage::HLSL ? EntryPoint : "main";
		}
		GpuShaderLanguage GetShaderLanguage() const { return ShaderLanguage; }
		GpuShaderModel GetShaderModelVer() const;

		virtual TArray<GpuShaderLayoutBinding> GetLayout() const = 0;

	public:
		GpuShaderCompilerFlag CompilerFlag = GpuShaderCompilerFlag::None;
		TArray<uint32> SpvCode;
        
    protected:
        ShaderType Type;
        FString ShaderName;
        FString EntryPoint;
		GpuShaderLanguage ShaderLanguage;
        FString SourceText;
		FString ProcessedSourceText;
        
        FString FileName;
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
        ShaderCandidateKind Kind;
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
		FString Desc;
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

	struct ShaderSymbol
	{
		ShaderTokenType Type;
		TArray<TPair<FString, ShaderTokenType>> Tokens;
		FString File;
		uint32 Row;
		FString Desc;
		FString Url;

		struct FuncOverload
		{
			TArray<TPair<FString, ShaderTokenType>> Tokens;
			TArray<ShaderParameter> Params;
		};
		TArray<FuncOverload> Overloads;
	};

    FRAMEWORK_API TArray<ShaderCandidateInfo> DefaultCandidates(GpuShaderLanguage Language, ShaderType Type);

    class FRAMEWORK_API ShaderTU
    {
	protected:
		ShaderTU(const FString& InShaderSource, const FString& InShaderName);

    public:
		static TUniquePtr<ShaderTU> Create(TRefCountPtr<GpuShader> InShader);
		virtual ~ShaderTU() = default;

		virtual TArray<ShaderDiagnosticInfo> GetDiagnostic() = 0;
		virtual ShaderTokenType GetTokenType(ShaderTokenType InType, uint32 Row, uint32 Col, uint32 Size) = 0;
		virtual TArray<ShaderCandidateInfo> GetCodeComplete(uint32 Row, uint32 Col) = 0;
		virtual TArray<ShaderOccurrence> GetOccurrences(uint32 Row, uint32 Col) = 0;
		virtual ShaderSymbol GetSymbolInfo(uint32 Row, uint32 Col, uint32 Size) = 0;
		virtual TArray<Vector2u> GetInactiveRegions() = 0;

		const TArray<ShaderFunc>& GetFuncs() const { return Funcs; };
		const TArray<ShaderScope>& GetGuideLineScopes() const { return GuideLineScopes; };
		FString GetStr(uint32 Row, uint32 Col, uint32 Size) const { 
			if ((int32)Row > LineRanges.Num() || (int32)Col > LineRanges[Row -1].Len())
			{
				return "";
			}
			return ShaderSource.Mid(LineRanges[Row - 1].BeginIndex + Col - 1, Size);
		}
        
    protected:
		FString ShaderSource;
		FString ShaderName;
		TArray<FTextRange> LineRanges;
		TArray<ShaderFunc> Funcs;
		TArray<ShaderScope> GuideLineScopes;
    };

}
