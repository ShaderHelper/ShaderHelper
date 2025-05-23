#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"

namespace FW
{
	class Dx12Shader : public GpuShader
	{
	public:
		Dx12Shader(const GpuShaderFileDesc& Desc);
		Dx12Shader(const GpuShaderSourceDesc& Desc);
        
        
    public:
		IDxcBlob* GetCompilationResult() const { return ByteCode; }
		virtual bool IsCompiled() const override { return ByteCode.IsValid(); }
		void SetCompilationResult(TRefCountPtr<IDxcBlob> InByteCode) { ByteCode = MoveTemp(InByteCode); }

	private:
		TRefCountPtr<IDxcBlob> ByteCode;
	};

	TRefCountPtr<Dx12Shader> CreateDx12Shader(const GpuShaderFileDesc& FileDesc);
    TRefCountPtr<Dx12Shader> CreateDx12Shader(const GpuShaderSourceDesc& SourceDesc);

	class DxcCompiler
	{
	public:
		DxcCompiler();
		bool Compile(TRefCountPtr<Dx12Shader> InShader, FString& OutErrorInfo, const TArray<FString>& Definitions) const;

		TRefCountPtr<IDxcCompiler3> Compiler;
		TRefCountPtr<IDxcUtils> CompierUitls;
	};

	inline DxcCompiler GShaderCompiler;
}
