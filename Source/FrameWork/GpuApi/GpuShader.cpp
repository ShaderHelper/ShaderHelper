#include "CommonHeader.h"
#include "GpuShader.h"

THIRD_PARTY_INCLUDES_START
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
	#include <windows.h>
	#include <ole2.h>
	#include "dxcisense.h"
#include "Windows/HideWindowsPlatformTypes.h"
#else
	#define BOOL RESOLVED_BOOL
	#include "dxcisense.h"
	#undef BOOL
#endif
THIRD_PARTY_INCLUDES_END

//ue regex is invalid if disable icu, so use the regex from std.
//TODO RE2
#include <regex>
#include <string>
namespace HLSL
{
    TArray<FString> KeyWords = {
        "register", "packoffset", "static", "const", "out", "in", "inout",
        "break", "continue", "discard", "do", "for", "if",
        "else", "switch", "while", "case", "default", "return", "true", "false",
        "SV_Coverage", "SV_Depth", "SV_DispatchThreadID", "SV_DomainLocation", "SV_GroupID", "SV_GroupIndex", "SV_GroupThreadID", "SV_GSInstanceID",
        "SV_InsideTessFactor", "SV_IsFrontFace", "SV_OutputControlPointID", "SV_POSITION", "SV_Position", "SV_RenderTargetArrayIndex",
        "SV_SampleIndex", "SV_TessFactor", "SV_ViewportArrayIndex", "SV_InstanceID", "SV_PrimitiveID", "SV_VertexID", "SV_TargetID",
        "SV_TARGET", "SV_Target", "SV_Target0", "SV_Target1", "SV_Target2", "SV_Target3", "SV_Target4", "SV_Target5", "SV_Target6", "SV_Target7",
		"template", "typename", "sizeof", "numthreads",
    };

    TArray<FString> BuiltinTypes = {
        "bool", "bool1", "bool2", "bool3", "bool4", "bool1x1", "bool1x2", "bool1x3", "bool1x4",
        "bool2x1", "bool2x2", "bool2x3", "bool2x4", "bool3x1", "bool3x2", "bool3x3", "bool3x4",
        "bool4x1", "bool4x2", "bool4x3", "bool4x4",
        "int", "int1", "int2", "int3", "int4", "int1x1", "int1x2", "int1x3", "int1x4",
        "int2x1", "int2x2", "int2x3", "int2x4", "int3x1", "int3x2", "int3x3", "int3x4",
        "int4x1", "int4x2", "int4x3", "int4x4",
        "uint", "uint1", "uint2", "uint3", "uint4", "uint1x1", "uint1x2", "uint1x3", "uint1x4",
        "uint2x1", "uint2x2", "uint2x3", "uint2x4", "uint3x1", "uint3x2", "uint3x3", "uint3x4",
        "uint4x1", "uint4x2", "uint4x3", "uint4x4",
        "dword", "dword1", "dword2", "dword3", "dword4", "dword1x1", "dword1x2", "dword1x3", "dword1x4",
        "dword2x1", "dword2x2", "dword2x3", "dword2x4", "dword3x1", "dword3x2", "dword3x3", "dword3x4",
        "dword4x1", "dword4x2", "dword4x3", "dword4x4",
        "half", "half1", "half2", "half3", "half4", "half1x1", "half1x2", "half1x3", "half1x4",
        "half2x1", "half2x2", "half2x3", "half2x4", "half3x1", "half3x2", "half3x3", "half3x4",
        "half4x1", "half4x2", "half4x3", "half4x4",
        "float", "float1", "float2", "float3", "float4", "float1x1", "float1x2", "float1x3", "float1x4",
        "float2x1", "float2x2", "float2x3", "float2x4", "float3x1", "float3x2", "float3x3", "float3x4",
        "float4x1", "float4x2", "float4x3", "float4x4",
        "double", "double1", "double2", "double3", "double4", "double1x1", "double1x2", "double1x3", "double1x4",
        "double2x1", "double2x2", "double2x3", "double2x4", "double3x1", "double3x2", "double3x3", "double3x4",
        "double4x1", "double4x2", "double4x3", "double4x4",
        "snorm", "unorm", "string", "void", "cbuffer", "struct",
        "Buffer", "AppendStructuredBfufer", "ByteAddressBuffer", "ConsumeStructuredBuffer", "StructuredBuffer",
        "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer", "RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D",
        "sampler", "sampler1D", "sampler2D", "sampler3D", "samplerCUBE", "SamplerComparisonState", "SamplerState",
        "texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS", "Texture2DMSArray", "Texture3D", "TextureCube",
    };

    TArray<FString> BuiltinFuncs = {
        "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble",
        "asfloat", "asin", "asint", "asuint", "atan", "atan2", "ceil", "clamp", "clip", "cos", "cosh", "countbits",
        "cross", "D3DCOLORtoUBYTE4", "ddx", "ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine",
        "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync",
        "distance", "dot", "dst", "EvaluateAttributeAtCentroid", "EvaluateAttributeAtSample",
        "EvaluateAttributeSnapped", "exp", "exp2", "f16tof32", "f32tof16", "faceforward", "firstbithigh",
        "firstbitlow", "floor", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount",
        "GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync",
        "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange", "InterlockedCompareStore",
        "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor",
        "isfinite", "isinf", "isnan", "ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "mul",
        "noise", "normalize", "pow", "Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax",
        "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg",
        "ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg",
        "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin", "radians", "rcp", "reflect", "refract",
        "reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step",
        "tan", "tanh", "tex1D", "tex1Dbias", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2Dbias",
        "tex2Dgrad", "tex2Dlod", "tex2Dproj", "tex3D", "tex3Dbias", "tex3Dgrad", "tex3Dlod", "tex3Dproj",
        "texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc",
    };
}

namespace FW
{
	GpuShaderPreProcessor& GpuShaderPreProcessor::ReplacePrintStringLiteral()
	{
//		std::string ShaderString{TCHAR_TO_UTF8(*ShaderText)};
//		std::smatch Match;
//		auto ReplaceMatch = [&](const std::regex& Pattern) {
//			std::size_t SearchPos = 0;
//			while (std::regex_search(ShaderString.cbegin() + SearchPos, ShaderString.cend(), Match, Pattern))
//			{
//				FString PrintStringLiteral = UTF8_TO_TCHAR(std::string{Match[1]}.data());
//				FString TextArr = "EXPAND(uint StrArr[] = {";
//				int Num = (int)PrintStringLiteral.Len() - 1;
//				for (int i = 1; i < Num;)
//				{
//					if (i + 1 < Num && PrintStringLiteral[i] == '\\')
//					{
//						//can not use std::format, std::to_chars needs a minimum deployment target of 13.4 on mac.
//						TextArr += FString::Printf(TEXT("'\\%c',"), PrintStringLiteral[i + 1]);
//						i += 2;
//					}
//					else
//					{
//						TextArr += FString::Printf(TEXT("'%c',"), PrintStringLiteral[i]);
//						i++;
//					}
//				}
//				TextArr += "'\\0'})";
//				ShaderString.replace(SearchPos + Match.position(1), Match[1].length(), TCHAR_TO_UTF8(*TextArr));
//				SearchPos += Match.position() + TextArr.Len();
//			}
//		};
//		static std::regex PrintRegex{"Print *\\( *(\".*\")"};
//		static std::regex PrintAtMouseRegex{"PrintAtMouse *\\( *(\".*\")"};
//		static std::regex AssertRegex{"Assert *\\(.*(\".*\")"};
//		ReplaceMatch(PrintRegex);
//		ReplaceMatch(PrintAtMouseRegex);
//      ReplaceMatch(AssertRegex);
//
//		ShaderText = FString{UTF8_TO_TCHAR(ShaderString.data())};
		FString NewShaderText;
		const TCHAR* Src = *ShaderText;
		int32 SrcLen = ShaderText.Len();
		int32 i = 0;

		auto TryReplace = [&](const TCHAR* Keyword) -> bool
		{
			int32 KeywordLen = FCString::Strlen(Keyword);
			if (FCString::Strncmp(Src + i, Keyword, KeywordLen) == 0)
			{
				int32 j = i + KeywordLen;
				while (j < SrcLen && FChar::IsWhitespace(Src[j])) ++j;
				if (j < SrcLen && Src[j] == '(')
				{
					++j;

					while (j < SrcLen && FChar::IsWhitespace(Src[j])) ++j;
					if (j < SrcLen && Src[j] == '"')
					{
						int32 StringStart = j;
						++j;

						while (j < SrcLen)
						{
							if (Src[j] == '"' && Src[j - 1] != '\\')
								break;
							++j;
						}
						if (j < SrcLen && Src[j] == '"')
						{
							int32 StringEnd = j;
							NewShaderText.AppendChars(Src + i, StringStart - i);
							//EXPAND(uint StrArr[] = {...})
							FString PrintStringLiteral = ShaderText.Mid(StringStart, StringEnd - StringStart + 1);
							FString TextArr = TEXT("EXPAND(uint StrArr[] = {");
							int Num = PrintStringLiteral.Len() - 1;
							for (int k = 1; k < Num;)
							{
								if (k + 1 < Num && PrintStringLiteral[k] == '\\')
								{
									TextArr += FString::Printf(TEXT("'\\%c',"), PrintStringLiteral[k + 1]);
									k += 2;
								}
								else
								{
									TextArr += FString::Printf(TEXT("'%c',"), PrintStringLiteral[k]);
									k++;
								}
							}
							TextArr += TEXT("'\\0'})");
							NewShaderText += TextArr;
							i = StringEnd + 1;
							return true;
						}
					}
				}
			}
			return false;
		};

		while (i < SrcLen)
		{
			if (TryReplace(TEXT("PrintAtMouse")) ||
				TryReplace(TEXT("Print")) ||
				TryReplace(TEXT("Assert")))
			{
				continue;
			}

			NewShaderText.AppendChar(Src[i]);
			++i;
		}

		ShaderText = MoveTemp(NewShaderText);
		return *this;
	}


	GpuShader::GpuShader(const GpuShaderFileDesc& FileDesc)
		: GpuResource(GpuResourceType::Shader)
		, Type(FileDesc.Type)
		, EntryPoint(FileDesc.EntryPoint)
        , FileName(FileDesc.FileName)
        , IncludeDirs(FileDesc.IncludeDirs)
	{
		ShaderName = FPaths::GetBaseFilename(*FileName);
		FString ShaderFileText;
		FFileHelper::LoadFileToString(ShaderFileText, **FileName);
		SourceText = FileDesc.ExtraDecl + ShaderFileText;
	}

	GpuShader::GpuShader(const GpuShaderSourceDesc& SourceDesc)
		: GpuResource(GpuResourceType::Shader)
		, Type(SourceDesc.Type)
		, ShaderName(SourceDesc.Name)
		, EntryPoint(SourceDesc.EntryPoint)
		, SourceText(SourceDesc.Source)
        , IncludeDirs(SourceDesc.IncludeDirs)
	{

	}

	GpuShaderModel GpuShader::GetShaderModelVer() const
	{
		GpuShaderModel ShaderModel;
		if (EnumHasAnyFlags(CompilerFlag, GpuShaderCompilerFlag::Enable16bitType))
		{
			ShaderModel.Minor = 2;
		}
		return ShaderModel;
	}

	FString AdjustDiagLineNumber(const FString& DiagInfo, int32 Delta)
	{
		std::string DiagString{TCHAR_TO_UTF8(*DiagInfo)};
		std::regex Pattern{":([0-9]+):[0-9]+: (?:error|warning):"};
		std::smatch Match;
		std::size_t SearchPos = 0;
		while (std::regex_search(DiagString.cbegin() + SearchPos, DiagString.cend(), Match, Pattern))
		{
			std::string RowStr = Match[1];
			std::string RowNumber = std::to_string(std::stoi(RowStr) + Delta);
			DiagString.replace(SearchPos + Match.position(1), Match[1].length(), RowNumber);
			SearchPos += Match.position() + RowNumber.length();
		}
		return FString{UTF8_TO_TCHAR(DiagString.data())};
	}

	TArray<ShaderDiagnosticInfo> ParseDiagnosticInfoFromDxc(FStringView HlslDiagnosticInfo)
    {
        TArray<ShaderDiagnosticInfo> Ret;

        int32 LineInfoFirstPos = HlslDiagnosticInfo.Find(TEXT("hlsl:"));
        while (LineInfoFirstPos != INDEX_NONE)
        {
			ShaderDiagnosticInfo DiagnosticInfo;

            int32 LineInfoLastPos = HlslDiagnosticInfo.Find(TEXT("\n"), LineInfoFirstPos);
            FStringView LineStringView{ HlslDiagnosticInfo.GetData() + LineInfoFirstPos, LineInfoLastPos - LineInfoFirstPos };

            int32 LineInfoFirstColonPos = 4;
            int32 Pos2 = LineStringView.Find(TEXT(":"), LineInfoFirstColonPos + 1);
			DiagnosticInfo.Row = FCString::Atoi(LineStringView.SubStr(LineInfoFirstColonPos + 1, Pos2 - LineInfoFirstColonPos - 1).GetData());
            int32 Pos3 = LineStringView.Find(TEXT(":"), Pos2 + 1);
			DiagnosticInfo.Col = FCString::Atoi(LineStringView.SubStr(Pos2 + 1, Pos3 - Pos2 - 1).GetData());

            int32 ErrorPos = LineStringView.Find(TEXT("error: "), Pos3);
            if (ErrorPos != INDEX_NONE)
            {
                int32 ErrorInfoEnd = LineStringView.Find(TEXT("["), ErrorPos + 7);
                if (ErrorInfoEnd == INDEX_NONE) {
                    ErrorInfoEnd = LineStringView.Len();
                }
				DiagnosticInfo.Error = LineStringView.SubStr(ErrorPos + 7, ErrorInfoEnd - ErrorPos - 7);
                Ret.Add(MoveTemp(DiagnosticInfo));
            }
			else
			{
				int32 WarnPos = LineStringView.Find(TEXT("warning: "), Pos3);
				int32 WarnInfoEnd = LineStringView.Find(TEXT("["), WarnPos + 9);
				if (WarnInfoEnd == INDEX_NONE) {
					WarnInfoEnd = LineStringView.Len();
				}
				DiagnosticInfo.Warn = LineStringView.SubStr(WarnPos + 9, WarnInfoEnd - WarnPos - 9);
				Ret.Add(MoveTemp(DiagnosticInfo));
			}

            LineInfoFirstPos = HlslDiagnosticInfo.Find(TEXT("hlsl:"), LineInfoLastPos);
        }

        return Ret;
    }

    struct ShaderTUImpl
    {
        TRefCountPtr<IDxcIntelliSense> ISense;
        TRefCountPtr<IDxcIndex> Index;
        TRefCountPtr<IDxcUnsavedFile> Unsaved;
        TRefCountPtr<IDxcTranslationUnit> TU;
    };

	ShaderTU::ShaderTU(FStringView HlslSource, const TArray<FString>& IncludeDirs)
        :Impl(MakePimpl<ShaderTUImpl>())
    {
        DxcCreateInstance(CLSID_DxcIntelliSense, IID_PPV_ARGS(Impl->ISense.GetInitReference()));
        Impl->ISense->CreateIndex(Impl->Index.GetInitReference());
		auto SourceText = StringCast<UTF8CHAR>(HlslSource.GetData());
        Impl->ISense->CreateUnsavedFile("Temp.hlsl", (char*)SourceText.Get(), SourceText.Length() * sizeof(UTF8CHAR), Impl->Unsaved.GetInitReference());
        
		TArray<const char*> DxcArgs;
		DxcArgs.Add("-HV");
		DxcArgs.Add("2021");
		//DxcArgs.Add("-D");
		//DxcArgs.Add("ENABLE_PRINT=0");
		TArray<FTCHARToUTF8> CharIncludeDirs;
        for (const FString& IncludeDir : IncludeDirs)
        {
            DxcArgs.Add("-I");
            CharIncludeDirs.Emplace(*IncludeDir);
            DxcArgs.Add(CharIncludeDirs.Last().Get());
        }

        DxcTranslationUnitFlags UnitFlag = DxcTranslationUnitFlags(DxcTranslationUnitFlags_UseCallerThread | DxcTranslationUnitFlags_DetailedPreprocessingRecord);
        Impl->Index->ParseTranslationUnit("Temp.hlsl", DxcArgs.GetData(), DxcArgs.Num(), AUX::GetAddrExt(Impl->Unsaved.GetReference()), 1, UnitFlag, Impl->TU.GetInitReference());
    }

    TArray<ShaderDiagnosticInfo> ShaderTU::GetDiagnostic()
    {
        uint32 NumDiag{};
        Impl->TU->GetNumDiagnostics(&NumDiag);
        FString Diags;
        for(uint32 i = 0; i < NumDiag; i++)
        {
            TRefCountPtr<IDxcDiagnostic> Diag;
            Impl->TU->GetDiagnostic(i, Diag.GetInitReference());
            char* DiagResult;
            DxcDiagnosticDisplayOptions Options = DxcDiagnosticDisplayOptions(DxcDiagnostic_DisplaySourceLocation | DxcDiagnostic_DisplayColumn | DxcDiagnostic_DisplaySeverity);
            Diag->FormatDiagnostic(Options, &DiagResult);
            Diags += DiagResult;
            Diags += "\n";
            CoTaskMemFree(DiagResult);
        }
        return ParseDiagnosticInfoFromDxc(Diags);
    }

    HLSL::CandidateKind MapCursorDecl(DxcCursorKind InDecl)
    {
        switch (InDecl)
        {
        case DxcCursor_VarDecl:
        case DxcCursor_ParmDecl:
        case DxcCursor_FieldDecl:
            return HLSL::CandidateKind::Var;
        case DxcCursor_MacroDefinition:
            return HLSL::CandidateKind::Macro;
        case DxcCursor_FunctionDecl:
        case DxcCursor_CXXMethod:
        case DxcCursor_FunctionTemplate:
            return HLSL::CandidateKind::Func;
        case DxcCursor_TypedefDecl:
        case DxcCursor_StructDecl:
            return HLSL::CandidateKind::Type;
        default:
            return HLSL::CandidateKind::Unknown;
        }
    }

    TArray<ShaderCandidateInfo> DefaultCandidates()
    {
        TArray<ShaderCandidateInfo> Candidates;
        
        for (const FString& Type : HLSL::BuiltinTypes)
        {
            Candidates.AddUnique({HLSL::CandidateKind::Type, Type});
        }
        for (const FString& Func : HLSL::BuiltinFuncs)
        {
            Candidates.AddUnique({HLSL::CandidateKind::Func, Func});
        }
        for (const FString& Keyword : HLSL::KeyWords)
        {
            Candidates.AddUnique({HLSL::CandidateKind::KeyWord, Keyword});
        }
        
        return Candidates;
    }

	HLSL::TokenType ShaderTU::GetTokenType(HLSL::TokenType InType, uint32 Row, uint32 Col)
	{
		TRefCountPtr<IDxcSourceLocation> SrcLoc;
		TRefCountPtr<IDxcFile> DxcFile;
		Impl->TU->GetFile("Temp.hlsl", DxcFile.GetInitReference());
		Impl->TU->GetLocation(DxcFile, Row, Col, SrcLoc.GetInitReference());
		
		TRefCountPtr<IDxcCursor> DxcCursor;
		DxcCursorKind CursorKind;
		Impl->TU->GetCursorForLocation(SrcLoc, DxcCursor.GetInitReference());
		DxcCursor->GetKind(&CursorKind);
		
		TRefCountPtr<IDxcCursor> DxcReferencedCursor;
		DxcCursorKind ReferencedCursorKind;
		DxcCursor->GetReferencedCursor(DxcReferencedCursor.GetInitReference());
		DxcReferencedCursor->GetKind(&ReferencedCursorKind);
		
		TRefCountPtr<IDxcCursor> DxcReferencedCursorLexPar;
		DxcCursorKind ReferencedCursorLexParKind;
		DxcReferencedCursor->GetLexicalParent(DxcReferencedCursorLexPar.GetInitReference());
		DxcReferencedCursorLexPar->GetKind(&ReferencedCursorLexParKind);
		
		BSTR CursorName;
		BSTR ReferencedCursorName;
		DxcCursor->GetDisplayName(&CursorName);
		DxcReferencedCursor->GetDisplayName(&ReferencedCursorName);
		LPSTR CursorSpelling;
		LPSTR ReferencedCursorSpelling;
		DxcCursor->GetSpelling(&CursorSpelling);
		DxcCursor->GetSpelling(&ReferencedCursorSpelling);
		
		if(InType == HLSL::TokenType::Identifier)
		{
			if(ReferencedCursorKind == DxcCursor_FunctionDecl || ReferencedCursorKind == DxcCursor_CXXMethod)
			{
				return HLSL::TokenType::Func;
			}
			
			if(ReferencedCursorKind == DxcCursor_ParmDecl)
			{
				return HLSL::TokenType::Parm;
			}
			
			if(ReferencedCursorKind == DxcCursor_StructDecl)
			{
				return HLSL::TokenType::Type;
			}
			
			if(ReferencedCursorKind == DxcCursor_VarDecl)
			{
				if(ReferencedCursorLexParKind == DxcCursor_FunctionDecl || ReferencedCursorLexParKind == DxcCursor_CXXMethod)
				{
					return HLSL::TokenType::LocalVar;
				}
				else
				{
					return HLSL::TokenType::Var;
				}
			}
			
			if(CursorKind == DxcCursor_MacroExpansion)
			{
				return HLSL::TokenType::Preprocess;
			}
		}

		
		return InType;
	}

	TArray<ShaderFuncScope> ShaderTU::GetFuncScopes()
	{
		TRefCountPtr<IDxcFile> DxcFile;
		Impl->TU->GetFile("Temp.hlsl", DxcFile.GetInitReference());
		
		TArray<ShaderFuncScope> Scopes;
		TRefCountPtr<IDxcCursor> DxcRootCursor;
		Impl->TU->GetCursor(DxcRootCursor.GetInitReference());
		
		unsigned int NumChildren = 0;
		IDxcCursor** Children = nullptr;
		DxcRootCursor->GetChildren(0, -1, &NumChildren, &Children);
		for (unsigned int i = 0; i < NumChildren; ++i)
		{
			IDxcCursor* ChildCursor = Children[i];
			DxcCursorKind Kind;
			ChildCursor->GetKind(&Kind);
			bool IsDefinition;
			ChildCursor->IsDefinition(&IsDefinition);
			if(IsDefinition && (Kind == DxcCursor_FunctionDecl || Kind == DxcCursor_CXXMethod))
			{
				TRefCountPtr<IDxcSourceRange> Extent;
				ChildCursor->GetExtent(Extent.GetInitReference());

				TRefCountPtr<IDxcSourceLocation> StartLoc, EndLoc;
				Extent->GetStart(StartLoc.GetInitReference());
				Extent->GetEnd(EndLoc.GetInitReference());

				TRefCountPtr<IDxcFile> CursorFile;
				unsigned StartLine, StartCol, EndLine, EndCol;
				StartLoc->GetSpellingLocation(CursorFile.GetInitReference(), &StartLine, &StartCol, nullptr);
				EndLoc->GetSpellingLocation(CursorFile.GetInitReference(), &EndLine, &EndCol, nullptr);
				bool IsInMainFile;
				CursorFile->IsEqualTo(DxcFile, &IsInMainFile);
				if(IsInMainFile)
				{
					LPSTR CursorName;
					ChildCursor->GetSpelling(&CursorName);
					
					Scopes.Emplace(ANSI_TO_TCHAR(CursorName), Vector2i{StartLine, StartCol}, Vector2i{EndLine, EndCol});
					CoTaskMemFree(CursorName);
				}
			}
			
			ChildCursor->Release();
		}
		
		CoTaskMemFree(Children);

		return Scopes;
	}

    TArray<ShaderCandidateInfo> ShaderTU::GetCodeComplete(uint32 Row, uint32 Col)
    {
        TArray<ShaderCandidateInfo> Candidates;
        
        TRefCountPtr<IDxcCodeCompleteResults> CodeCompleteResults;
        uint32 NumResult{};
        Impl->TU->CodeCompleteAt("Temp.hlsl", Row, Col, AUX::GetAddrExt(Impl->Unsaved.GetReference()), 1, DxcCodeCompleteFlags_IncludeMacros, CodeCompleteResults.GetInitReference());
        CodeCompleteResults->GetNumResults(&NumResult);
        for(uint32 i = 0; i < NumResult; i++)
        {
            TRefCountPtr<IDxcCompletionResult> CompletionResult;
            TRefCountPtr<IDxcCompletionString> CompletionString;
            CodeCompleteResults->GetResultAt(i, CompletionResult.GetInitReference());
            
            DxcCursorKind Kind;
            CompletionResult->GetCursorKind(&Kind);
            
            HLSL::CandidateKind CandidateKind = MapCursorDecl(Kind);
            if(CandidateKind != HLSL::CandidateKind::Unknown)
            {
                //SH_LOG(LogTemp,Display,TEXT("-------%d-------"),  Kind);
                CompletionResult->GetCompletionString(CompletionString.GetInitReference());
                uint32 NumChunk{};
                CompletionString->GetNumCompletionChunks(&NumChunk);
                for(uint32 ChunkNumber = 0; ChunkNumber < NumChunk; ChunkNumber++)
                {
                    DxcCompletionChunkKind CompletionChunkKind;
                    CompletionString->GetCompletionChunkKind(ChunkNumber, &CompletionChunkKind);
                    char* CompletionText;
                    CompletionString->GetCompletionChunkText(ChunkNumber, &CompletionText);
                    if(CompletionChunkKind == DxcCompletionChunk_ResultType)
                    {
                        if(FString{CompletionText}.Contains(TEXT("__attribute__((ext_vector_type(")))
                        {
                            break;
                        }
                    }
                    
                    //SH_LOG(LogTemp,Display,TEXT("----%d----"), CompletionChunkKind);
                    if(CompletionChunkKind == DxcCompletionChunk_TypedText)
                    {
                        //SH_LOG(LogTemp,Display,TEXT("%s"), ANSI_TO_TCHAR(CompletionText));
                        Candidates.AddUnique({CandidateKind, CompletionText});
                    }
                    CoTaskMemFree(CompletionText);
                }
            }
            
   
        }
        
        return Candidates;
    }

}
