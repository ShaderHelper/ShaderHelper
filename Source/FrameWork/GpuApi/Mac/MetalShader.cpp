#include "CommonHeader.h"
#include "MetalShader.h"
#include "MetalDevice.h"
#include "ShaderConductor.hpp"

namespace FRAMEWORK
{
    TRefCountPtr<MetalShader> CreateMetalShader(ShaderType InType, FString InSourceText, FString ShaderName, FString InEntryPoint)
    {
        return new MetalShader(InType, MoveTemp(InSourceText), MoveTemp(ShaderName), MoveTemp(InEntryPoint));
    }

    TRefCountPtr<MetalShader> CreateMetalShader(FString FileName, ShaderType InType, FString ExtraDeclaration, FString EntryPoint)
    {
        return new MetalShader(MoveTemp(FileName), InType, MoveTemp(ExtraDeclaration), MoveTemp(EntryPoint));
    }

    bool CompileShader(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo)
    {
        ns::AutoReleasedError Err;
        mtlpp::CompileOptions CompOpt;
        mtlpp::Library ByteCodeLib = GDevice.NewLibrary(TCHAR_TO_ANSI(*InShader->GetSourceText()), CompOpt, &Err);
        
        if(!ByteCodeLib) {
            SH_LOG(LogMetal, Error, TEXT("Shader compilation failed: %s"), ConvertOcError(Err.GetPtr()));
			OutErrorInfo = ConvertOcError(Err.GetPtr());
            return false;
        }
        
        mtlpp::Function ByteCodeFunc = ByteCodeLib.NewFunction(TCHAR_TO_ANSI(*InShader->GetEntryPoint()));
        if(!ByteCodeFunc) {
            SH_LOG(LogMetal, Error, TEXT("Shader compilation failed: EntryPoint not found: %s "), *InShader->GetEntryPoint());
            return false;
        }
        InShader->SetCompilationResult(MoveTemp(ByteCodeFunc));
        return true;
    }

    static ShaderConductor::ShaderStage MapShaderCunductorStage(ShaderType InType)
    {
        switch(InType)
        {
        case ShaderType::VertexShader:         return ShaderConductor::ShaderStage::VertexShader;
        case ShaderType::PixelShader:          return ShaderConductor::ShaderStage::PixelShader;
        default:
            SH_LOG(LogMetal, Fatal, TEXT("Invalid ShaderType."));
            return ShaderConductor::ShaderStage::VertexShader;
        }
    }

    bool CompileShaderFromHlsl(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo)
    {
        ShaderConductor::Compiler::SourceDesc SourceDesc{};
        
        SourceDesc.stage = MapShaderCunductorStage(InShader->GetShaderType());
        
        //len + 1 for copying null terminator
        TArray<char> EntryPointAnsi{TCHAR_TO_ANSI(*InShader->GetEntryPoint()), InShader->GetEntryPoint().Len() + 1};
        SourceDesc.entryPoint = EntryPointAnsi.GetData();
        
        TArray<char> SourceAnsi{TCHAR_TO_ANSI(*InShader->GetSourceText()), InShader->GetSourceText().Len() + 1};
        SourceDesc.source = SourceAnsi.GetData();
        
        ShaderConductor::MacroDefine SpvMslOptions[] = {
            {"argument_buffers", "1"},
            {"enable_decoration_binding", "1"},
            {"force_active_argument_buffer_resources","1"}
        };
        
        ShaderConductor::Compiler::TargetDesc TargetDesc{};
        TargetDesc.language = ShaderConductor::ShadingLanguage::Msl_macOS;
		TargetDesc.version = "20200";
        TargetDesc.options = SpvMslOptions;
        TargetDesc.numOptions = UE_ARRAY_COUNT(SpvMslOptions);
        
        TArray<const char*> DxcArgs;
        DxcArgs.Add("-fspv-preserve-bindings");
        
        DxcArgs.Add("/T");
        std::string ShaderTarget{TCHAR_TO_ANSI(*InShader->GetShaderTarget())};
        DxcArgs.Add(ShaderTarget.data());
        
        TArray<std::string> AnsiIncludeDirs;
        for(const FString& IncludeDir : InShader->GetIncludeDirs())
        {
            AnsiIncludeDirs.Emplace(TCHAR_TO_ANSI(*IncludeDir));
        }
        for(const std::string& AnsiIncludeDir : AnsiIncludeDirs)
        {
            DxcArgs.Add("-I");
            DxcArgs.Add(AnsiIncludeDir.data());
        }
        
        ShaderConductor::Compiler::Options SCOptions;
        SCOptions.DXCArgs = DxcArgs.GetData();
        SCOptions.numDXCArgs = DxcArgs.Num();
        
        const ShaderConductor::Compiler::ResultDesc Result = ShaderConductor::Compiler::Compile(SourceDesc, SCOptions, TargetDesc);
        if(Result.errorWarningMsg.Size() > 0)
        {
            FString ErrorInfo = static_cast<const char*>(Result.errorWarningMsg.Data());
            SH_LOG(LogMetal, Error, TEXT("Shader cross compilation failed: %s"), *ErrorInfo);
			OutErrorInfo = MoveTemp(ErrorInfo);
            return false;
        }
        
        FString MslSourceText = {static_cast<const char*>(Result.target.Data()), (int32)Result.target.Size()};
        TRefCountPtr<MetalShader> TempShader = new MetalShader{InShader->GetShaderType(), MoveTemp(MslSourceText), {}, InShader->GetEntryPoint()};
        
        FString MslErrorInfo;
        bool IsSuccessfullyCompiledMsl = CompileShader(TempShader, MslErrorInfo);
        
        if(!IsSuccessfullyCompiledMsl)
        {
            return false;
        }
        
        InShader->SetCompilationResult(TempShader->GetCompilationResult());
        return true;

    }
}
