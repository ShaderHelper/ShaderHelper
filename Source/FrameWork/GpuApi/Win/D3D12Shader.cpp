#include "CommonHeader.h"
#include "D3D12Shader.h"

#pragma comment (lib, "dxcompiler.lib")

namespace FRAMEWORK
{
	 DxcCompiler::DxcCompiler()
	 {
		 DxCheck(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(Compiler.GetInitReference())));
		 DxCheck(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(CompilerLibrary.GetInitReference())));
	 }

	 bool DxcCompiler::Compile(TRefCountPtr<Dx12Shader> InShader) const
	 {
		TRefCountPtr<IDxcBlobEncoding> BlobEncoding;
		TRefCountPtr<IDxcOperationResult> OperationResult;
		auto SourceText = StringCast<ANSICHAR>(*InShader->GetSourceText());
		const ANSICHAR* SourceTextPtr = SourceText.Get();
		DxCheck(CompilerLibrary->CreateBlobWithEncodingFromPinned(SourceTextPtr, FCStringAnsi::Strlen(SourceTextPtr), CP_UTF8, BlobEncoding.GetInitReference()));
		bool IsCompilationSucceeded = SUCCEEDED(Compiler->Compile(BlobEncoding,
			*InShader->ShaderName, *InShader->EntryPoint, *InShader->ShaderTaget, 
			nullptr, 0, nullptr, 0, nullptr, 
			OperationResult.GetInitReference())
		);

		if (IsCompilationSucceeded) 
		{
			TRefCountPtr<IDxcBlob> ShaderBlob;
			DxCheck(OperationResult->GetResult(ShaderBlob.GetInitReference()));
			InShader->SetCompilationResult(ShaderBlob);
		}
		else 
		{
			if (OperationResult.IsValid()) 
			{
				TRefCountPtr<IDxcBlobEncoding> BlobEncodingError;
				OperationResult->GetErrorBuffer(BlobEncodingError.GetInitReference());
				FString ErrorStr{ (int32)BlobEncodingError->GetBufferSize(), (ANSICHAR*)BlobEncodingError->GetBufferPointer()};

				SH_LOG(LogDx12, Error, TEXT("Shader compilation failed: %s"), *ErrorStr);
			}
		}
		
		return IsCompilationSucceeded;
	 }



}