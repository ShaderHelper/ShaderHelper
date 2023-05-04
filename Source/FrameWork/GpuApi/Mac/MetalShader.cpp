#include "CommonHeader.h"
#include "MetalShader.h"

namespace FRAMEWORK
{
    bool CompileShader(TRefCountPtr<MetalShader> InShader)
    {
        ns::AutoReleasedError Err;
        mtlpp::CompileOptions CompOpt;
        const ns::String ShaderSrc = [NSString stringWithUTF8String:InShader->GetSourceText()];
        mtlpp::Library ByteCodeLib = g_device.NewLibrary(MoveTemp(ShaderSrc), CompOpt, &Err);
        
        if(!ByteCodeLib) {
            SH_LOG(LogMetal, Error, TEXT("Shader compilation failed: %s"), ConvertOcError(Err.GetPtr()));
        }
        
        mtlpp::Function ByteCodeFunc = ByteCodeLib.NewFunction(*InShader->GetEntryPoint());
        InShader->SetCompilationResult(MoveTemp(ByteCodeFunc));
    }
}
