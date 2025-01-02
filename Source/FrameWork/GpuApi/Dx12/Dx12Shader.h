#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	class Dx12Shader : public GpuShader
	{
	public:
        using GpuShader::GpuShader;
        
    public:
		IDxcBlob* GetCompilationResult() const { return ByteCode; }
		virtual bool IsCompiled() const override { return ByteCode.IsValid(); }
		void SetCompilationResult(TRefCountPtr<IDxcBlob> InByteCode) { ByteCode = MoveTemp(InByteCode); }

	private:
		TRefCountPtr<IDxcBlob> ByteCode;
	};

	TRefCountPtr<Dx12Shader> CreateDx12Shader(FString FileName, ShaderType InType, FString ExtraDeclaration, FString EntryPoint);
    TRefCountPtr<Dx12Shader> CreateDx12Shader(ShaderType InType, FString InSourceText, FString ShaderName, FString InEntryPoint);

	class DxcCompiler
	{
	public:
		DxcCompiler();
		bool Compile(TRefCountPtr<Dx12Shader> InShader, FString& OutErrorInfo) const;

	private:
		TRefCountPtr<IDxcCompiler3> Compiler;
		TRefCountPtr<IDxcUtils> CompierUitls;
		TRefCountPtr<IDxcIncludeHandler> CompilerIncludeHandler;
	};

	inline DxcCompiler GShaderCompiler;
}
