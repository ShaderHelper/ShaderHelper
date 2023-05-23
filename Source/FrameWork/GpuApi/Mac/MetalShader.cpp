#include "CommonHeader.h"
#include "MetalShader.h"
#include "MetalDevice.h"
#include "ShaderConductor.hpp"

namespace FRAMEWORK
{
    TRefCountPtr<MetalShader> CreateMetalShader(ShaderType InType, FString InSourceText, FString ShaderName, FString InEntryPoint)
    {
        return new MetalShader(MoveTemp(InType), MoveTemp(InSourceText), MoveTemp(ShaderName), MoveTemp(InEntryPoint));
    }

    bool CompileShader(TRefCountPtr<MetalShader> InShader)
    {
        ns::AutoReleasedError Err;
        mtlpp::CompileOptions CompOpt;
        mtlpp::Library ByteCodeLib = GDevice.NewLibrary(TCHAR_TO_ANSI(*InShader->GetSourceText()), CompOpt, &Err);
        
        if(!ByteCodeLib) {
            SH_LOG(LogMetal, Error, TEXT("Shader compilation failed: %s"), ConvertOcError(Err.GetPtr()));
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

    bool CompileShaderFromHlsl(TRefCountPtr<MetalShader> InShader)
    {
        ShaderConductor::Compiler::SourceDesc SourceDesc{};
        
        SourceDesc.stage = MapShaderCunductorStage(InShader->GetShaderType());
        
        //len + 1 for copying null terminator
        TArray<char> EntryPointAnsi{TCHAR_TO_ANSI(*InShader->GetEntryPoint()), InShader->GetEntryPoint().Len() + 1};
        SourceDesc.entryPoint = EntryPointAnsi.GetData();
        
        TArray<char> SourceAnsi{TCHAR_TO_ANSI(*InShader->GetSourceText()), InShader->GetSourceText().Len() + 1};
        SourceDesc.source = SourceAnsi.GetData();
        
        ShaderConductor::Compiler::TargetDesc TargetDesc{};
        TargetDesc.language = ShaderConductor::ShadingLanguage::Msl_macOS;
        
        const ShaderConductor::Compiler::ResultDesc Result = ShaderConductor::Compiler::Compile(MoveTemp(SourceDesc), {}, MoveTemp(TargetDesc));
        if(Result.errorWarningMsg.Size() > 0)
        {
            FString ErrorInfo = static_cast<const char*>(Result.errorWarningMsg.Data());
            SH_LOG(LogMetal, Error, TEXT("Shader cross compilation failed: %s"), *ErrorInfo);
            return false;
        }
        
        FString MslSourceText = {static_cast<const char*>(Result.target.Data()), (int32)Result.target.Size()};
        TRefCountPtr<MetalShader> TempShader = new MetalShader{InShader->GetShaderType(), MoveTemp(MslSourceText), {}, InShader->GetEntryPoint()};
        bool IsSuccessfullyCompiledMsl = CompileShader(TempShader);
        
        if(!IsSuccessfullyCompiledMsl)
        {
            return false;
        }
        
        InShader->SetCompilationResult(TempShader->GetCompilationResult());
        return true;

    }
}
