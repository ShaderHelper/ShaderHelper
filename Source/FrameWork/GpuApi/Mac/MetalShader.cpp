#include "CommonHeader.h"
#include "MetalShader.h"
#include "MetalDevice.h"

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
}
