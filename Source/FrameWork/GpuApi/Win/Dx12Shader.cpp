#include "CommonHeader.h"
#include "Dx12Shader.h"
#include "Common/Path/PathHelper.h"
#include <Misc/FileHelper.h>
#pragma comment (lib, "dxcompiler.lib")

namespace FRAMEWORK
{
	Dx12Shader::Dx12Shader(FString InFileName, ShaderType InType, const FString& ExtraDeclaration, FString InShaderTaget, FString InEntryPoint)
		: Type(InType)
		, FileName(MoveTemp(InFileName))
		, EntryPoint(MoveTemp(InEntryPoint))
		, ShaderTaget(MoveTemp(InShaderTaget))
	{
		ShaderName = FPaths::GetBaseFilename(*FileName);
		FString ShaderFileText;
		FFileHelper::LoadFileToString(ShaderFileText, **FileName);
		SourceText = ExtraDeclaration + MoveTemp(ShaderFileText);

		IncludeDirs.Add(FPaths::GetPath(*FileName));
	}

	Dx12Shader::Dx12Shader(ShaderType InType, FString InSourceText, FString InShaderName, FString InShaderTaget, FString InEntryPoint)
		: Type(InType)
		, ShaderName(MoveTemp(InShaderName))
		, EntryPoint(MoveTemp(InEntryPoint))
		, SourceText(MoveTemp(InSourceText))
		, ShaderTaget(MoveTemp(InShaderTaget))
	{
	}

	FString DecideShaderTarget(ShaderType InType)
	{
		FString ShaderTarget;
		if (InType == ShaderType::VertexShader)
		{
			ShaderTarget = "vs_6_0";

		}
		else if (InType == ShaderType::PixelShader)
		{
			ShaderTarget = "ps_6_0";
		}
		else
		{
			check(false);
		}
		return ShaderTarget;
	}

	TRefCountPtr<Dx12Shader> CreateDx12Shader(FString FileName, ShaderType InType, FString ExtraDeclaration, FString EntryPoint)
	{
		return new Dx12Shader( MoveTemp(FileName), InType, MoveTemp(ExtraDeclaration), DecideShaderTarget(InType), MoveTemp(EntryPoint));
	}

	TRefCountPtr<Dx12Shader> CreateDx12Shader(ShaderType InType, FString InSourceText, FString ShaderName, FString InEntryPoint)
    {
        return new Dx12Shader(MoveTemp(InType), MoveTemp(InSourceText), MoveTemp(ShaderName), DecideShaderTarget(InType), MoveTemp(InEntryPoint));
    }

	DxcCompiler::DxcCompiler()
	 {
		 DxCheck(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(Compiler.GetInitReference())));
		 DxCheck(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(CompierUitls.GetInitReference())));
		 DxCheck(CompierUitls->CreateDefaultIncludeHandler(CompilerIncludeHandler.GetInitReference()));
	 }

	 bool DxcCompiler::Compile(TRefCountPtr<Dx12Shader> InShader, FString& OutErrorInfo) const
	 {
		TRefCountPtr<IDxcBlobEncoding> BlobEncoding;
		TRefCountPtr<IDxcResult> CompileResult;
		auto SourceText = StringCast<ANSICHAR>(*InShader->GetSourceText());
		const ANSICHAR* SourceTextPtr = SourceText.Get();
		DxCheck(CompierUitls->CreateBlobFromPinned(SourceTextPtr, FCStringAnsi::Strlen(SourceTextPtr), CP_UTF8, BlobEncoding.GetInitReference()));

		TArray<const TCHAR*> Arguments;
		Arguments.Add(TEXT("/Qstrip_debug"));
		Arguments.Add(TEXT("/E"));
		Arguments.Add(*InShader->GetEntryPoint());
		Arguments.Add(TEXT("/T"));
		Arguments.Add(*InShader->GetShaderTarget());
#if DEBUG_SHADER
		Arguments.Add(TEXT("/Zi"));
#endif

		for(const FString& IncludeDir : InShader->GetIncludeDirs())
		{
			Arguments.Add(TEXT("-I"));
			Arguments.Add(*IncludeDir);
		}

		
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
			Arguments.GetData(), Arguments.Num(), CompilerIncludeHandler,
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
					OutErrorInfo = MoveTemp(ErrorStr);
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
				const FString PdbFile = PathHelper::SavedShaderDir() / PdbName;
				
				TArray<uint8> BinaryData{ (uint8*)PdbBlob->GetBufferPointer(), (int32)PdbBlob->GetBufferSize() };
				FFileHelper::SaveArrayToFile(BinaryData, *PdbFile);
#endif
			}
		}
		
		return IsCompilationSucceeded;
	 }

}
