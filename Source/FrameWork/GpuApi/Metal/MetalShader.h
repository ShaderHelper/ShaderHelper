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
        MTL::Function* GetCompilationResult() const { return ByteCodeFunc.get(); }
        const FString& GetMslText() const { return MslText; }
        
        virtual bool IsCompiled() const override { return (bool)ByteCodeFunc; }
        void SetCompilationResult(MTLFunctionPtr InByteCodeFunc) { ByteCodeFunc = MoveTemp(InByteCodeFunc); }
        void SetMslText(FString InText) { MslText = MoveTemp(InText); }
        
    private:
        MTLFunctionPtr ByteCodeFunc;
        FString MslText;
    };
    
    TRefCountPtr<MetalShader> CreateMetalShader(FString FileName, ShaderType InType, FString ExtraDeclaration,
                                                FString EntryPoint);
    TRefCountPtr<MetalShader> CreateMetalShader(ShaderType InType, FString InSourceText, FString ShaderName,
                                                FString InEntryPoint);

    bool CompileShader(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo);
    
    bool CompileShaderFromHlsl(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo);
}

