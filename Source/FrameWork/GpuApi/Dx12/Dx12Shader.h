#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	class Dx12Shader : public GpuShader
	{
	public:
		Dx12Shader(const FString& InFileName, ShaderType InType, const FString& ExtraDeclaration, const FString& InEntryPoint);
		Dx12Shader(ShaderType InType, const FString& InSourceText, const FString& InShaderName, const FString& InEntryPoint);
        
        
    public:
		IDxcBlob* GetCompilationResult() const { return ByteCode; }
		virtual bool IsCompiled() const override { return ByteCode.IsValid(); }
		void SetCompilationResult(TRefCountPtr<IDxcBlob> InByteCode) { ByteCode = MoveTemp(InByteCode); }

	private:
		TRefCountPtr<IDxcBlob> ByteCode;
	};

	TRefCountPtr<Dx12Shader> CreateDx12Shader(const FString& FileName, ShaderType InType, const FString& ExtraDeclaration, const FString& EntryPoint);
    TRefCountPtr<Dx12Shader> CreateDx12Shader(ShaderType InType, const FString& InSourceText, const FString& ShaderName, const FString& InEntryPoint);

	class DxcCompiler
	{
	public:
		DxcCompiler();
		bool Compile(TRefCountPtr<Dx12Shader> InShader, FString& OutErrorInfo) const;

		TRefCountPtr<IDxcCompiler3> Compiler;
		TRefCountPtr<IDxcUtils> CompierUitls;
	};

	inline DxcCompiler GShaderCompiler;
}
