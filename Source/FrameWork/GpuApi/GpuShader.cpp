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
#include <regex>
#include <string>

namespace FW
{
	GpuShaderPreProcessor& GpuShaderPreProcessor::ReplacePrintStringLiteral()
	{
		std::string ShaderString{TCHAR_TO_UTF8(*ShaderText)};
		std::smatch Match;

		auto ReplaceMatch = [&](const std::regex& Pattern) {
			while (std::regex_search(ShaderString, Match, Pattern))
			{
				FString PrintStringLiteral = UTF8_TO_TCHAR(std::string{Match[1]}.data());
				FString TextArr = "EXPAND(uint StrArr[] = {";
				int Num = (int)PrintStringLiteral.Len() - 1;
				for (int i = 1; i < Num;)
				{
					if (i + 1 < Num && PrintStringLiteral[i] == '\\')
					{
						//can not use std::format, std::to_chars needs a minimum deployment target of 13.4 on mac.
						TextArr += FString::Printf(TEXT("'\\%c',"), PrintStringLiteral[i + 1]);
						i += 2;
					}
					else
					{
						TextArr += FString::Printf(TEXT("'%c',"), PrintStringLiteral[i]);
						i++;
					}
				}
				TextArr += "'\\0'})";
				ShaderString.replace(Match.position(1), Match[1].length(), TCHAR_TO_UTF8(*TextArr));
			}
		};
		ReplaceMatch(std::regex{"Print *\\( *(\".*\")"});
		ReplaceMatch(std::regex{ "PrintAtMouse *\\( *(\".*\")" });

		ShaderText = FString{UTF8_TO_TCHAR(ShaderString.data())};
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
		if (HasFlag(GpuShaderFlag::Enable16bitType))
		{
			ShaderModel.Minor = 2;
		}
		return ShaderModel;
	}

    TArray<ShaderErrorInfo> ParseErrorInfoFromDxc(FStringView HlslErrorInfo)
    {
        TArray<ShaderErrorInfo> Ret;

        int32 LineInfoFirstPos = HlslErrorInfo.Find(TEXT("hlsl:"));
        while (LineInfoFirstPos != INDEX_NONE)
        {
            ShaderErrorInfo ErrorInfo;

            int32 LineInfoLastPos = HlslErrorInfo.Find(TEXT("\n"), LineInfoFirstPos);
            FStringView LineStringView{ HlslErrorInfo.GetData() + LineInfoFirstPos, LineInfoLastPos - LineInfoFirstPos };

            int32 LineInfoFirstColonPos = 4;
            int32 Pos2 = LineStringView.Find(TEXT(":"), LineInfoFirstColonPos + 1);
            ErrorInfo.Row = FCString::Atoi(LineStringView.SubStr(LineInfoFirstColonPos + 1, Pos2 - LineInfoFirstColonPos - 1).GetData());
            int32 Pos3 = LineStringView.Find(TEXT(":"), Pos2 + 1);
            ErrorInfo.Col = FCString::Atoi(LineStringView.SubStr(Pos2 + 1, Pos3 - Pos2 - 1).GetData());

            int32 ErrorPos = LineStringView.Find(TEXT("error: "), Pos3);
            if (ErrorPos != INDEX_NONE)
            {
                int32 ErrorInfoEnd = LineStringView.Find(TEXT("["), ErrorPos + 7);
                if (ErrorInfoEnd == INDEX_NONE) {
                    ErrorInfoEnd = LineStringView.Len();
                }
                ErrorInfo.Info = LineStringView.SubStr(ErrorPos + 7, ErrorInfoEnd - ErrorPos - 7);
                Ret.Add(MoveTemp(ErrorInfo));
            }

            LineInfoFirstPos = HlslErrorInfo.Find(TEXT("hlsl:"), LineInfoLastPos);
        }

        return Ret;
    }

    struct ISenseTUImpl
    {
        TRefCountPtr<IDxcIntelliSense> ISense;
        TRefCountPtr<IDxcIndex> Index;
        TRefCountPtr<IDxcUnsavedFile> Unsaved;
        TRefCountPtr<IDxcTranslationUnit> TU;
    };

    ISenseTU::ISenseTU(TRefCountPtr<GpuShader> InShader)
        :Impl(MakePimpl<ISenseTUImpl>())
    {
        DxcCreateInstance(CLSID_DxcIntelliSense, IID_PPV_ARGS(Impl->ISense.GetInitReference()));
        Impl->ISense->CreateIndex(Impl->Index.GetInitReference());
		auto SourceText = StringCast<UTF8CHAR>(*InShader->GetProcessedSourceText());
        Impl->ISense->CreateUnsavedFile("Temp.hlsl", (char*)SourceText.Get(), SourceText.Length() * sizeof(UTF8CHAR), Impl->Unsaved.GetInitReference());
        
		TArray<const char*> DxcArgs;
		DxcArgs.Add("-HV");
		DxcArgs.Add("2021");
		DxcArgs.Add("-D");
		DxcArgs.Add("ENABLE_PRINT=0");
		for (const FString& IncludeDir : InShader->GetIncludeDirs())
		{
			DxcArgs.Add("-I");
			DxcArgs.Add(TCHAR_TO_ANSI(*IncludeDir));
		}

        DxcTranslationUnitFlags UnitFlag = DxcTranslationUnitFlags(DxcTranslationUnitFlags_UseCallerThread);
        Impl->Index->ParseTranslationUnit("Temp.hlsl", DxcArgs.GetData(), DxcArgs.Num(), AUX::GetAddrExt(Impl->Unsaved.GetReference()), 1, UnitFlag, Impl->TU.GetInitReference());
    }

    TArray<ShaderErrorInfo> ISenseTU::GetDiagnostic()
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
        return ParseErrorInfoFromDxc(Diags);
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
        
        for (const char* Type : HLSL::BuiltinTypes)
        {
            Candidates.AddUnique({HLSL::CandidateKind::Type, Type});
        }
        for (const char* Func : HLSL::BuiltinFuncs)
        {
            Candidates.AddUnique({HLSL::CandidateKind::Func, Func});
        }
        for (const char* Keyword : HLSL::KeyWords)
        {
            Candidates.AddUnique({HLSL::CandidateKind::KeyWord, Keyword});
        }
        
        return Candidates;
    }

    TArray<ShaderCandidateInfo> ISenseTU::GetCodeComplete(uint32 Row, uint32 Col)
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
