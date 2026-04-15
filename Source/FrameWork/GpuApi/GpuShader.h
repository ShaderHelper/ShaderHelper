#pragma once
#include "GpuResourceCommon.h"
#include "Common/Path/PathHelper.h"
#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "GpuBindGroupLayout.h"
#include "ShaderConductor.hpp"

#include <Misc/FileHelper.h>

DECLARE_LOG_CATEGORY_EXTERN(LogShader, Log, All);
inline DEFINE_LOG_CATEGORY(LogShader);

namespace FW
{

	inline ShaderConductor::ShaderStage MapShaderCunductorStage(ShaderType InType)
	{
		switch (InType)
		{
		case ShaderType::Vertex:         return ShaderConductor::ShaderStage::VertexShader;
		case ShaderType::Pixel:          return ShaderConductor::ShaderStage::PixelShader;
		case ShaderType::Compute:        return ShaderConductor::ShaderStage::ComputeShader;
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
		Type,
		File
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
		CompileFromSpvCode = 1u << 2,
		SkipBindingShift = 1u << 3,
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
		TFunction<FString(const FString&)> IncludeHandler;
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
		TFunction<FString(const FString&)> IncludeHandler;
    };

	struct GpuShaderUbMemberInfo
	{
		FString Name;
		FString Type;
		uint32 Offset;
		uint32 Size;
	};

	struct GpuShaderVertexInput
	{
		uint32 Location = 0;
		FString Name;
		FString SemanticName;
		uint32 SemanticIndex = 0;
		FString Type;
	};

	struct GpuShaderStageSemantic
	{
		FString SemanticName;
		uint32 SemanticIndex = 0;
		uint32 Location = 0;
		bool bRead = false;
		bool bWritten = false;
	};

	inline bool IsShaderMatrix4x4Type(const FString& InType) { return InType == TEXT("float4x4") || InType == TEXT("mat4") || InType == TEXT("mat4x4"); }
	inline bool IsShaderVector4Type(const FString& InType)   { return InType == TEXT("float4") || InType == TEXT("vec4"); }
	inline bool IsShaderVector3Type(const FString& InType)   { return InType == TEXT("float3") || InType == TEXT("vec3"); }
	inline bool IsShaderVector2Type(const FString& InType)   { return InType == TEXT("float2") || InType == TEXT("vec2"); }
	inline bool IsShaderIntVector4Type(const FString& InType)  { return InType == TEXT("int4") || InType == TEXT("ivec4"); }
	inline bool IsShaderIntVector3Type(const FString& InType)  { return InType == TEXT("int3") || InType == TEXT("ivec3"); }
	inline bool IsShaderIntVector2Type(const FString& InType)  { return InType == TEXT("int2") || InType == TEXT("ivec2"); }
	inline bool IsShaderUintVector4Type(const FString& InType) { return InType == TEXT("uint4") || InType == TEXT("uvec4"); }
	inline bool IsShaderUintVector3Type(const FString& InType) { return InType == TEXT("uint3") || InType == TEXT("uvec3"); }
	inline bool IsShaderUintVector2Type(const FString& InType) { return InType == TEXT("uint2") || InType == TEXT("uvec2"); }
	inline bool IsShaderBoolType(const FString& InType)        { return InType == TEXT("bool"); }
	inline bool IsShaderBoolVector4Type(const FString& InType) { return InType == TEXT("bool4") || InType == TEXT("bvec4"); }
	inline bool IsShaderBoolVector3Type(const FString& InType) { return InType == TEXT("bool3") || InType == TEXT("bvec3"); }
	inline bool IsShaderBoolVector2Type(const FString& InType) { return InType == TEXT("bool2") || InType == TEXT("bvec2"); }

	inline GpuFormat ShaderTypeToGpuFormat(const FString& InType)
	{
		if (IsShaderVector4Type(InType) || IsShaderMatrix4x4Type(InType)) return GpuFormat::R32G32B32A32_FLOAT;
		if (IsShaderVector3Type(InType))   return GpuFormat::R32G32B32_FLOAT;
		if (IsShaderVector2Type(InType))   return GpuFormat::R32G32_FLOAT;
		if (InType == TEXT("float"))        return GpuFormat::R32_FLOAT;
		if (IsShaderUintVector4Type(InType) || IsShaderIntVector4Type(InType)) return GpuFormat::R32G32B32A32_UINT;
		if (IsShaderUintVector3Type(InType) || IsShaderIntVector3Type(InType)) return GpuFormat::R32G32B32_UINT;
		if (IsShaderUintVector2Type(InType) || IsShaderIntVector2Type(InType)) return GpuFormat::R32G32_UINT;
		if (InType == TEXT("uint") || InType == TEXT("int")) return GpuFormat::R32_UINT;
		return GpuFormat::R32G32B32A32_FLOAT;
	}

	struct GpuShaderLayoutBinding
	{
		FString Name;
		int32 Slot;
		BindingGroupSlot Group;
		BindingType Type;
		BindingShaderStage Stage;
		TArray<GpuShaderUbMemberInfo> UbMembers;
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
		const TFunction<FString(const FString&)>& GetIncludeHandler() const { return IncludeHandler; }
        const FString& GetShaderName() const { return ShaderName; }
        ShaderType GetShaderType() const {return Type;}
        const FString& GetSourceText() const { return SourceText; }
		const FString& GetProcessedSourceText() const { return ProcessedSourceText; }
		FString GetEntryPoint() const {
			return ShaderLanguage == GpuShaderLanguage::HLSL ? EntryPoint : "main";
		}
		GpuShaderLanguage GetShaderLanguage() const { return ShaderLanguage; }
		GpuShaderModel GetShaderModelVer() const;

		virtual TArray<GpuShaderLayoutBinding> GetLayout() const;
		virtual TArray<GpuShaderVertexInput> GetVertexInputs() const;
		virtual TArray<GpuShaderStageSemantic> GetStageOutputSemantics() const;
		virtual TArray<GpuShaderStageSemantic> GetStageInputSemantics() const;

		friend uint32 GetTypeHash(const GpuShader& Shader)
		{
			uint32 Hash = ::GetTypeHash(Shader.SourceText);
			for (const FString& Arg : Shader.CompileExtraArgs)
			{
				Hash = HashCombine(Hash, ::GetTypeHash(Arg));
			}
			return Hash;
		}

	public:
		GpuShaderCompilerFlag CompilerFlag = GpuShaderCompilerFlag::None;
		TArray<uint32> SpvCode;
		TArray<FString> CompileExtraArgs;
        
    protected:
        ShaderType Type;
        FString ShaderName;
        FString EntryPoint;
		GpuShaderLanguage ShaderLanguage;
        FString SourceText;
		FString ProcessedSourceText;
        
        FString FileName;
        TArray<FString> IncludeDirs;
		TFunction<FString(const FString&)> IncludeHandler;
    };

	struct ShaderDiagnosticInfo
	{
		FString File;
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
		FString File;
		Vector2i Start;
		Vector2i End;
		TArray<ShaderParameter> Params;
	};

	struct ShaderOccurrence
	{
		uint32 Row, Col;
		bool operator==(const ShaderOccurrence&) const = default;
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
		TArray<TPair<FString, ShaderTokenType>> ExpandedTokens;
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
		FString GetLineStr(uint32 Row) const {
			if ((int32)Row > LineRanges.Num())
			{
				return "";
			}
			return ShaderSource.Mid(LineRanges[Row - 1].BeginIndex, LineRanges[Row - 1].Len());
		}
        
    protected:
		FString ShaderSource;
		FString ShaderName;
		TArray<FTextRange> LineRanges;
		TArray<ShaderFunc> Funcs;
		TArray<ShaderScope> GuideLineScopes;
    };

}
