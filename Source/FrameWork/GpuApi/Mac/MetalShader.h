#pragma once
#include "GpuApi/GpuResource.h"
#include "MetalCommon.h"

namespace FRAMEWORK
{
    class MetalShader : public GpuShader
    {
    public:
        using GpuShader::GpuShader;
        
    public:
        id<MTLFunction> GetCompilationResult() const { return ByteCodeFunc; }
        const FString& GetMslText() const { return MslText; }
        
        virtual bool IsCompiled() const override { return (bool)ByteCodeFunc; }
        void SetCompilationResult(mtlpp::Function InByteCodeFunc) { ByteCodeFunc = MoveTemp(InByteCodeFunc); }
        void SetMslText(FString InText) { MslText = MoveTemp(InText); }
        
    private:
        mtlpp::Function ByteCodeFunc;
        FString MslText;
    };
    
    TRefCountPtr<MetalShader> CreateMetalShader(FString FileName, ShaderType InType, FString ExtraDeclaration,
                                                FString EntryPoint);
    TRefCountPtr<MetalShader> CreateMetalShader(ShaderType InType, FString InSourceText, FString ShaderName,
                                                FString InEntryPoint);

    bool CompileShader(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo);
    
    bool CompileShaderFromHlsl(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo);
}

