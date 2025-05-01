#pragma once
#include "GpuApi/GpuResource.h"
#include "MetalCommon.h"

namespace FW
{
    class MetalShader : public GpuShader
    {
    public:
		MetalShader(const FString& InFileName, ShaderType InType, const FString& ExtraDeclaration, const FString& InEntryPoint);
		MetalShader(ShaderType InType, const FString& InSourceText, const FString& InShaderName, const FString& InEntryPoint);
        
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
    
    TRefCountPtr<MetalShader> CreateMetalShader(const FString& FileName, ShaderType InType, const FString& ExtraDeclaration,
		const FString& EntryPoint);
    TRefCountPtr<MetalShader> CreateMetalShader(ShaderType InType, const FString& InSourceText, const FString& ShaderName,
		const FString& InEntryPoint);

    bool CompileShader(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo);
    
    bool CompileShaderFromHlsl(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo);
}

