#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
	class Dx12Shader : public GpuShader
	{
	public:
		Dx12Shader(FString InFileName, ShaderType InType, const FString& ExtraDeclaration, FString InShaderTaget, FString InEntryPoint);
		Dx12Shader(ShaderType InType, FString InSourceText, FString InShaderName, FString InShaderTaget, FString InEntryPoint);
        
    public:
		TOptional<FString> GetFileName() const { return FileName; }
		const TArray<FString>& GetIncludeDirs() const { return IncludeDirs; }
		const FString& GetSourceText() const { return SourceText; }
        const FString& GetEntryPoint() const { return EntryPoint; }
        const FString& GetShaderTarget() const { return ShaderTaget; }
		IDxcBlob* GetCompilationResult() const { return ByteCode; }
		virtual bool IsCompiled() const override { return ByteCode.IsValid(); }
		void SetCompilationResult(TRefCountPtr<IDxcBlob> InByteCode) { ByteCode = MoveTemp(InByteCode); }

	private:
        ShaderType Type;
        FString ShaderName;
        FString EntryPoint;
        FString SourceText;
        FString ShaderTaget;
		TRefCountPtr<IDxcBlob> ByteCode;

		TOptional<FString> FileName;
		TArray<FString> IncludeDirs;
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
