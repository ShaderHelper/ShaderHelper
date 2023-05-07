#pragma once
#include "D3D12Common.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
	class Dx12Shader : public GpuShader
	{
	public:
		Dx12Shader(ShaderType InType, FString InSourceText, FString InShaderName, FString InShaderTaget, FString InEntryPoint)
			: Type(InType)
			, ShaderName(MoveTemp(InShaderName))
			, EntryPoint(MoveTemp(InEntryPoint))
			, SourceText(MoveTemp(InSourceText))
            , ShaderTaget(MoveTemp(InShaderTaget))
		{
		}
        
    public:
		const FString& GetSourceText() const { return SourceText; }
        const FString& GetEntryPoint() const { return EntryPoint; }
        const FString& GetShaderTarget() const { return ShaderTaget; }
		IDxcBlob* GetCompilationResult() const { return ByteCode; }
		bool IsCompiled() const { return ByteCode.IsValid(); }
		void SetCompilationResult(TRefCountPtr<IDxcBlob> InByteCode) { ByteCode = MoveTemp(InByteCode); }

	private:
        ShaderType Type;
        FString ShaderName;
        FString EntryPoint;
        FString SourceText;
        FString ShaderTaget;
		TRefCountPtr<IDxcBlob> ByteCode;
	};

    TRefCountPtr<Dx12Shader> CreateDx12Shader(ShaderType InType, FString InSourceText, FString ShaderName,
                                              FString InEntryPoint = "main");

	class DxcCompiler
	{
	public:
		DxcCompiler();
		bool Compile(TRefCountPtr<Dx12Shader> InShader) const;

	private:
		TRefCountPtr<IDxcLibrary> CompilerLibrary;
		TRefCountPtr<IDxcCompiler3> Compiler;
	};

	inline DxcCompiler GShaderCompiler;
}
