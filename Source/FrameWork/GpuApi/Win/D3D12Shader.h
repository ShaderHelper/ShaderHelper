#pragma once
#include "D3D12Common.h"
#include "GpuApi/GpuResource.h"

namespace FRAMEWORK
{
	class Dx12Shader : public GpuShader
	{
	public:
		Dx12Shader(
			ShaderType InType, FString InSourceText, FString ShaderName, 
			FString InEntryPoint = "main"
		)
			: Type(InType)
			, ShaderName(MoveTemp(ShaderName))
			, EntryPoint(MoveTemp(InEntryPoint))
			, SourceText(MoveTemp(InSourceText))
		{
			if (Type == ShaderType::VertexShader) { ShaderTaget = "vs_6_0"; }
			if (Type == ShaderType::PixelShader) { ShaderTaget = "ps_6_0"; }
		}
		const FString& GetSourceText() const { return SourceText; }
		IDxcBlob* GetCompilationResult() const { return ByteCode; }
		bool IsCompiled() const { return ByteCode.IsValid(); }
		void SetCompilationResult(TRefCountPtr<IDxcBlob> InByteCode) { ByteCode = MoveTemp(InByteCode); }

	public:
		ShaderType Type;
		FString ShaderName;
		FString EntryPoint;
		FString ShaderTaget;

	private:
		FString SourceText;
		TRefCountPtr<IDxcBlob> ByteCode;
	};

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
