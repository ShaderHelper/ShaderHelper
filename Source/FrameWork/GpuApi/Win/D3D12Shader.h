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
			FString InEntryPoint = "main", 
			FString InShaderTarget = "PS_5_0"
		)
			: Type(InType)
			, ShaderName(MoveTemp(ShaderName))
			, EntryPoint(MoveTemp(InEntryPoint))
			, ShaderTaget(MoveTemp(InShaderTarget))
			, SourceText(MoveTemp(InSourceText))
		{}
		const ANSICHAR* GetSourceText() const { return TCHAR_TO_ANSI(*SourceText); }
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
		TRefCountPtr<IDxcCompiler2> Compiler;
	};

	inline DxcCompiler GShaderCompiler;
}