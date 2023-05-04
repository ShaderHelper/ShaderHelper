#include "CommonHeader.h"
#include "D3D12Shader.h"
#include "Common/Path/PathHelper.h"
#include <Misc/FileHelper.h>
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
		TRefCountPtr<IDxcResult> CompileResult;
		auto SourceText = StringCast<ANSICHAR>(*InShader->GetSourceText());
		const ANSICHAR* SourceTextPtr = SourceText.Get();
		DxCheck(CompilerLibrary->CreateBlobWithEncodingFromPinned(SourceTextPtr, FCStringAnsi::Strlen(SourceTextPtr), CP_UTF8, BlobEncoding.GetInitReference()));

		TArray<const TCHAR*> Arguments;
		Arguments.Add(TEXT("/Qstrip_debug"));
		Arguments.Add(TEXT("/E"));
		Arguments.Add(*InShader->GetEntryPoint());
		Arguments.Add(TEXT("/T"));
		Arguments.Add(*InShader->GetShaderTarget());
#if DEBUG_SHADER
		Arguments.Add(TEXT("/Zi"));
#endif
		
		DxcBuffer SourceBuffer = { 0 };
		SourceBuffer.Ptr = BlobEncoding->GetBufferPointer();
		SourceBuffer.Size = BlobEncoding->GetBufferSize();
		BOOL bKnown = 0;
		uint32 Encoding = 0;
		if (SUCCEEDED(BlobEncoding->GetEncoding(&bKnown, (uint32*)&Encoding)))
		{
			if (bKnown)
			{
				SourceBuffer.Encoding = Encoding;
			}
		}
		bool IsApiSucceeded = SUCCEEDED(Compiler->Compile(&SourceBuffer,
			Arguments.GetData(), Arguments.Num(), nullptr,
			IID_PPV_ARGS(CompileResult.GetInitReference()))
		);

		bool IsCompilationSucceeded = IsApiSucceeded;

		if (IsApiSucceeded)
		{

			if (CompileResult.IsValid())
			{
				TRefCountPtr<IDxcBlobEncoding> BlobEncodingError;
				CompileResult->GetErrorBuffer(BlobEncodingError.GetInitReference());
				FString ErrorStr{ (int32)BlobEncodingError->GetBufferSize(), (ANSICHAR*)BlobEncodingError->GetBufferPointer() };
				if (!ErrorStr.IsEmpty())
				{
					SH_LOG(LogDx12, Error, TEXT("Shader compilation failed: %s"), *ErrorStr);
					IsCompilationSucceeded = false;
				}
			}

			if (IsCompilationSucceeded)
			{
				TRefCountPtr<IDxcBlob> ShaderBlob;
				DxCheck(CompileResult->GetResult(ShaderBlob.GetInitReference()));
				InShader->SetCompilationResult(ShaderBlob);
#if DEBUG_SHADER
				TRefCountPtr<IDxcBlob> PdbBlob;
				TRefCountPtr<IDxcBlobUtf16> PdbNameBlob;
				DxCheck(CompileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(PdbBlob.GetInitReference()), PdbNameBlob.GetInitReference()));
				const FString PdbName = PdbNameBlob->GetStringPointer();
				const FString PdbFile = PathHelper::SavedDir() + PdbName;
				
				TArray<uint8> BinaryData{ (uint8*)PdbBlob->GetBufferPointer(), (int32)PdbBlob->GetBufferSize() };
				FFileHelper::SaveArrayToFile(BinaryData, *PdbFile);
#endif
			}
		}
		
		return IsCompilationSucceeded;
	 }



}
