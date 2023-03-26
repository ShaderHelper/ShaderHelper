#include "CommonHeader.h"
#include "D3D12Shader.h"

namespace FRAMEWORK
{
	 const FString FullScreenVsText = R"(
		void main(
		in uint VertID : SV_VertexID,
		out float4 Pos : SV_Position,
		out float2 Tex : TexCoord0
		)
		{
			Tex = float2(uint2(VertID, VertID << 1) & 2);
			Pos = float4(lerp(float2(-1, 1), float2(1, -1), Tex), 0, 1);
		}
	)";

	 const FString PsForTest = R"(
		float4 main() : SV_Target 
		{
			return float4(1,1,0,0);
		}
	)";

	 DxcCompiler::DxcCompiler()
	 {
		 DxCheck(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(Compiler.GetInitReference())));
		 DxCheck(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(CompilerLibrary.GetInitReference())));
	 }

	 bool DxcCompiler::Compile(TRefCountPtr<Dx12Shader> InShader) const
	 {
		TRefCountPtr<IDxcBlobEncoding> BlobEncoding;
		TRefCountPtr<IDxcOperationResult> OperationResult;
		const ANSICHAR* SourceText = InShader->GetSourceText();
		DxCheck(CompilerLibrary->CreateBlobWithEncodingFromPinned(SourceText, FCStringAnsi::Strlen(SourceText), CP_UTF8, BlobEncoding.GetInitReference()));
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
				FString ErrorStr{ BlobEncodingError->GetBufferSize(), (ANSICHAR*)BlobEncodingError->GetBufferPointer()};

				SH_LOG(LogDx12, Error, TEXT("Shader compilation failed: %s"), *ErrorStr);
			}
		}
		
		return IsCompilationSucceeded;
	 }



}