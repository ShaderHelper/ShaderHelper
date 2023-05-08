#pragma once
#include "GpuApi/GpuResource.h"
#include "MetalCommon.h"

namespace FRAMEWORK
{
    class MetalShader : public GpuShader
    {
    public:
        MetalShader(ShaderType InType, FString InSourceText, FString ShaderName, FString InEntryPoint)
            : Type(InType)
            , ShaderName(MoveTemp(ShaderName))
            , EntryPoint(MoveTemp(InEntryPoint))
            , SourceText(MoveTemp(InSourceText))
        {
            
        }
        
    public:
        const FString& GetSourceText() const { return SourceText; }
        const FString& GetEntryPoint() const { return EntryPoint; }
        id<MTLFunction> GetCompilationResult() const { return ByteCodeFunc; }
        
        bool IsCompiled() const { return (bool)ByteCodeFunc; }
        void SetCompilationResult(mtlpp::Function InByteCodeFunc) { ByteCodeFunc = MoveTemp(InByteCodeFunc); }
        
    private:
        ShaderType Type;
        FString ShaderName;
        FString EntryPoint;
        FString SourceText;
        mtlpp::Function ByteCodeFunc;
    };
    
    TRefCountPtr<MetalShader> CreateMetalShader(ShaderType InType, FString InSourceText, FString ShaderName,
                                                FString InEntryPoint);
    bool CompileShader(TRefCountPtr<MetalShader> InShader);
}

