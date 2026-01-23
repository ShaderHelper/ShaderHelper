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
        
		TArray<GpuShaderLayoutBinding> GetLayout() const override;
    public:
		IDxcBlob* GetCompilationResult() const { return ByteCode; }
		virtual bool IsCompiled() const override { return ByteCode.IsValid(); }
		void SetCompilationResult(TRefCountPtr<IDxcBlob> InByteCode) { ByteCode = MoveTemp(InByteCode); }

		// Shader cache
		bool TryLoadCachedByteCode(const TArray<FString>& ExtraArgs);
		void SaveByteCodeToCache(const TArray<FString>& ExtraArgs) const;
		FString GetCacheFilePath(const TArray<FString>& ExtraArgs) const;
		uint32 ComputeSourceHash(const TArray<FString>& ExtraArgs) const;

	private:
		TRefCountPtr<IDxcBlob> ByteCode;
	};

	TRefCountPtr<Dx12Shader> CreateDx12Shader(const GpuShaderFileDesc& FileDesc);
    TRefCountPtr<Dx12Shader> CreateDx12Shader(const GpuShaderSourceDesc& SourceDesc);

	class DxcCompiler
	{
	public:
		DxcCompiler();
		bool Compile(TRefCountPtr<Dx12Shader> InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs) const;

		TRefCountPtr<IDxcCompiler3> Compiler;
		TRefCountPtr<IDxcUtils> CompilerUitls;
	};

	inline DxcCompiler GShaderCompiler;
}
