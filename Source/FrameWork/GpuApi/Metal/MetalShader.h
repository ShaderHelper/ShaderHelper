#pragma once
#include "GpuApi/GpuResource.h"
#include "MetalCommon.h"

namespace FW
{
    class MetalShader : public GpuShader
    {
    public:
        MetalShader(const GpuShaderFileDesc& Desc);
        MetalShader(const GpuShaderSourceDesc& Desc);

		TArray<GpuShaderLayoutBinding> GetLayout() const override;
    public:
        MTL::Function* GetCompilationResult() const { return ByteCodeFunc.get(); }
        const FString& GetMslText() const { return MslText; }
        
        virtual bool IsCompiled() const override { return (bool)ByteCodeFunc; }
        void SetCompilationResult(MTLFunctionPtr InByteCodeFunc) { ByteCodeFunc = MoveTemp(InByteCodeFunc); }
        void SetMslText(FString InText) { MslText = MoveTemp(InText); }
        
    public:
        Vector3u ThreadGroupSize{};
        
    private:
        MTLFunctionPtr ByteCodeFunc;
        FString MslText;

    };
    
    TRefCountPtr<MetalShader> CreateMetalShader(const GpuShaderFileDesc& FileDesc);
    TRefCountPtr<MetalShader> CreateMetalShader(const GpuShaderSourceDesc& SourceDesc);
    
    bool CompileMetalShader(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs);
}

