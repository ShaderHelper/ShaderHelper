#include "CommonHeader.h"
#include "GpuShader.h"
#include "Common/Path/PathHelper.h"

THIRD_PARTY_INCLUDES_START
#define BOOL RESOLVED_BOOL
#include "dxcisense.h"
#undef BOOL
THIRD_PARTY_INCLUDES_END

namespace FW
{

	GpuShader::GpuShader(FString InFileName, ShaderType InType, const FString& ExtraDeclaration, FString InEntryPoint)
		: GpuResource(GpuResourceType::Shader)
		, Type(InType)
		, FileName(MoveTemp(InFileName))
		, EntryPoint(MoveTemp(InEntryPoint))
	{
		ShaderName = FPaths::GetBaseFilename(*FileName);
		FString ShaderFileText;
		FFileHelper::LoadFileToString(ShaderFileText, **FileName);
		SourceText = ExtraDeclaration + MoveTemp(ShaderFileText);

		IncludeDirs.Add(PathHelper::ShaderDir());
		IncludeDirs.Add(FPaths::GetPath(*FileName));
	}

	GpuShader::GpuShader(ShaderType InType, FString InSourceText, FString InShaderName, FString InEntryPoint)
		: GpuResource(GpuResourceType::Shader)
		, Type(InType)
		, ShaderName(MoveTemp(InShaderName))
		, EntryPoint(MoveTemp(InEntryPoint))
		, SourceText(MoveTemp(InSourceText))
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

    ISenseTU::ISenseTU(FStringView HlslSource)
        :Impl(MakePimpl<ISenseTUImpl>())
    {
        DxcCreateInstance(CLSID_DxcIntelliSense, IID_PPV_ARGS(Impl->ISense.GetInitReference()));
        Impl->ISense->CreateIndex(Impl->Index.GetInitReference());
        Impl->ISense->CreateUnsavedFile("Temp.hlsl", TCHAR_TO_ANSI(HlslSource.GetData()), HlslSource.Len(), Impl->Unsaved.GetInitReference());
        
        DxcTranslationUnitFlags UnitFlag = DxcTranslationUnitFlags(DxcTranslationUnitFlags_UseCallerThread);
        Impl->Index->ParseTranslationUnit("Temp.hlsl", nullptr, 0, AUX::GetAddrExt(Impl->Unsaved.GetReference()), 1, UnitFlag, Impl->TU.GetInitReference());
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
            //SH_LOG(LogTemp,Display,TEXT("-------%d-------"),  Kind);
            HLSL::CandidateKind CandidateKind = MapCursorDecl(Kind);
            if(CandidateKind != HLSL::CandidateKind::Unknown)
            {
                CompletionResult->GetCompletionString(CompletionString.GetInitReference());
                uint32 NumChunk{};
                CompletionString->GetNumCompletionChunks(&NumChunk);
                for(uint32 ChunkNumber = 0; ChunkNumber < NumChunk; ChunkNumber++)
                {
                    DxcCompletionChunkKind CompletionChunkKind;
                    CompletionString->GetCompletionChunkKind(ChunkNumber, &CompletionChunkKind);
                    //SH_LOG(LogTemp,Display,TEXT("----%d----"), CompletionChunkKind);
                    if(CompletionChunkKind == DxcCompletionChunk_TypedText)
                    {
                        char* CompletionText;
                        CompletionString->GetCompletionChunkText(ChunkNumber, &CompletionText);
                        //SH_LOG(LogTemp,Display,TEXT("%s"), ANSI_TO_TCHAR(CompletionText));
                        Candidates.AddUnique({CandidateKind, CompletionText});
                        CoTaskMemFree(CompletionText);
                    }
                }
            }
            
   
        }
        
        return Candidates;
    }

}
