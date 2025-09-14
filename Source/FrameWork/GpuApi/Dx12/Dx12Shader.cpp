#include "CommonHeader.h"
#include "Dx12Shader.h"
#include "Common/Path/PathHelper.h"
#include <Misc/FileHelper.h>
#include "GpuApi/GpuFeature.h"
#include "ShaderConductor.hpp"

namespace FW
{
	Dx12Shader::Dx12Shader(const GpuShaderFileDesc& Desc)
		: GpuShader(Desc)
	{
		ProcessedSourceText = GpuShaderPreProcessor{ SourceText }
			.ReplacePrintStringLiteral()
			.Finalize();
	}

	Dx12Shader::Dx12Shader(const GpuShaderSourceDesc& Desc)
		: GpuShader(Desc)
	{
		ProcessedSourceText = GpuShaderPreProcessor{ SourceText }
			.ReplacePrintStringLiteral()
			.Finalize();
	}

	TRefCountPtr<Dx12Shader> CreateDx12Shader(const GpuShaderFileDesc& FileDesc)
	{
		return new Dx12Shader(FileDesc);
	}

	TRefCountPtr<Dx12Shader> CreateDx12Shader(const GpuShaderSourceDesc& SourceDesc)
    {
        return new Dx12Shader(SourceDesc);
    }

	class ShIncludeHandler final : public IDxcIncludeHandler
	{
	public:
		ShIncludeHandler(TRefCountPtr<Dx12Shader> InShader) : Shader(MoveTemp(InShader)) {}

	public:
		HRESULT STDMETHODCALLTYPE QueryInterface(
			/* [in] */ REFIID riid,
			/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR* __RPC_FAR* ppvObject) override
		{
			if (IsEqualIID(riid, __uuidof(IDxcIncludeHandler)))
			{
				*ppvObject = dynamic_cast<IDxcIncludeHandler*>(this);
				this->AddRef();
				return S_OK;
			}
			else if (IsEqualIID(riid, __uuidof(IUnknown)))
			{
				*ppvObject = dynamic_cast<IUnknown*>(this);
				this->AddRef();
				return S_OK;
			}
			else
			{
				return E_NOINTERFACE;
			}
		}

		ULONG STDMETHODCALLTYPE AddRef(void) override
		{
			++RefCount;
			return RefCount;
		}

		ULONG STDMETHODCALLTYPE Release(void) override
		{
			--RefCount;
			ULONG result = RefCount;
			if (result == 0)
			{
				delete this;
			}
			return result;
		}


		HRESULT STDMETHODCALLTYPE LoadSource(
			_In_z_ LPCWSTR pFilename,
			_COM_Outptr_result_maybenull_ IDxcBlob** ppIncludeSource
		) override
		{
			for (const FString& IncludeDir : Shader->GetIncludeDirs())
			{
				FString IncludedFile = FPaths::Combine(IncludeDir, pFilename);
				if (IFileManager::Get().FileExists(*IncludedFile))
				{
					FString ShaderText;
					FFileHelper::LoadFileToString(ShaderText, *IncludedFile);
					ShaderText = GpuShaderPreProcessor{ ShaderText }
						.ReplacePrintStringLiteral()
						.Finalize();
					auto SourceText = StringCast<UTF8CHAR>(*ShaderText);
					GShaderCompiler.CompilerUitls->CreateBlob(SourceText.Get(), SourceText.Length() * sizeof(UTF8CHAR), CP_UTF8,
						reinterpret_cast<IDxcBlobEncoding**>(ppIncludeSource));
					return S_OK;
				}
			}
			return E_FAIL;
		}

	private:
		TRefCountPtr<Dx12Shader> Shader;
		std::atomic<ULONG> RefCount = 0;
	};

	DxcCompiler::DxcCompiler()
	 {
		 DxCheck(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(Compiler.GetInitReference())));
		 DxCheck(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(CompilerUitls.GetInitReference())));
	 }

	FString GetShaderProfile(ShaderType InType, GpuShaderModel InModel)
	{
		FString ProfileName;
		if (InType == ShaderType::VertexShader) {
			ProfileName += "vs";
		}
		else if (InType == ShaderType::PixelShader) {
			ProfileName += "ps";
		}
		else if (InType == ShaderType::ComputeShader) {
			ProfileName += "cs";
		}

		ProfileName += "_";
		ProfileName += FString::FromInt(InModel.Major);
		ProfileName += "_";
		ProfileName += FString::FromInt(InModel.Minor);

		return ProfileName;
	}

	 bool DxcCompiler::Compile(TRefCountPtr<Dx12Shader> InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs) const
	 {
		TRefCountPtr<IDxcBlobEncoding> BlobEncoding;
		TRefCountPtr<IDxcResult> CompileResult;
		auto SourceText = StringCast<UTF8CHAR>(*InShader->GetProcessedSourceText());
		DxCheck(CompilerUitls->CreateBlobFromPinned(SourceText.Get(), SourceText.Length() * sizeof(UTF8CHAR), CP_UTF8, BlobEncoding.GetInitReference()));

		TArray<const TCHAR*> Arguments;
		Arguments.Add(TEXT("/Qstrip_debug"));
		Arguments.Add(TEXT("/E"));
		Arguments.Add(*InShader->GetEntryPoint());

		FString ShaderProfile = GetShaderProfile(InShader->GetShaderType(), InShader->GetShaderModelVer());
		Arguments.Add(TEXT("/T"));
		Arguments.Add(*ShaderProfile);
#if DEBUG_SHADER
		Arguments.Add(TEXT("/Zi"));
        Arguments.Add(TEXT("/Od"));
#endif
        //Arguments.Add(TEXT("-no-warnings"));

		if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::GenSpvForDebugging))
		{
			Arguments.Add(TEXT("/Od"));
			Arguments.Add(TEXT("-fcgl"));
			Arguments.Add(TEXT("-spirv"));
			Arguments.Add(TEXT("-Vd"));
			Arguments.Add(TEXT("-fvk-use-dx-layout"));
			Arguments.Add(TEXT("-fspv-debug=vulkan-with-source"));
		}

		ValidateGpuFeature(GpuFeature::Support16bitType, TEXT("Hardware does not support 16bitType, shader model <= 6.2"))
		{
			if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::Enable16bitType))
			{
				Arguments.Add(TEXT("-enable-16bit-types"));
				Arguments.Add(TEXT("-D"));
				Arguments.Add(TEXT("ENABLE_16BIT_TYPE"));
			}
		};
        Arguments.Add(TEXT("-D"));
        Arguments.Add(TEXT("FINAL_HLSL"));
		for (const FString& ExtraArg : ExtraArgs)
		{
			Arguments.Add(*ExtraArg);
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
		FString ShaderName = InShader->GetShaderName();
#if DEBUG_SHADER
		FFileHelper::SaveStringToFile(InShader->GetProcessedSourceText(), *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".hlsl"));
#endif

		TRefCountPtr<IDxcIncludeHandler> IncludeHandler = new ShIncludeHandler(InShader);
		bool IsApiSucceeded = SUCCEEDED(Compiler->Compile(&SourceBuffer,
			Arguments.GetData(), Arguments.Num(), IncludeHandler,
			IID_PPV_ARGS(CompileResult.GetInitReference()))
		);

		bool IsCompilationSucceeded = IsApiSucceeded;

		if (IsApiSucceeded)
		{

			if (CompileResult.IsValid())
			{
				TRefCountPtr<IDxcBlobEncoding> BlobEncodingError;
				CompileResult->GetErrorBuffer(BlobEncodingError.GetInitReference());
				FString DiagnosticInfo{ (int32)BlobEncodingError->GetBufferSize(), (ANSICHAR*)BlobEncodingError->GetBufferPointer() };
				HRESULT ResultStatus;
				CompileResult->GetStatus(&ResultStatus);
				if(FAILED(ResultStatus))
				{
					//SH_LOG(LogShader, Error, TEXT("Compilation failed: %s"), *DiagnosticInfo);
					OutErrorInfo = MoveTemp(DiagnosticInfo);
					IsCompilationSucceeded = false;
				}
				else
				{
					OutWarnInfo = MoveTemp(DiagnosticInfo);
				}
			
			}

			if (IsCompilationSucceeded)
			{
				TRefCountPtr<IDxcBlob> ShaderBlob;
				DxCheck(CompileResult->GetResult(ShaderBlob.GetInitReference()));
				if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::GenSpvForDebugging))
				{
					TArray<uint32> SpvCode = { (uint32*)ShaderBlob->GetBufferPointer(), (int)ShaderBlob->GetBufferSize() / 4 };
					InShader->SpvCode = MoveTemp(SpvCode);
#if DEBUG_SHADER
					ShaderConductor::Compiler::DisassembleDesc SpvDisassembleDesc{
						.language = ShaderConductor::ShadingLanguage::SpirV,
						.binary = (uint8*)ShaderBlob->GetBufferPointer(),
						.binarySize = (uint32_t)ShaderBlob->GetBufferSize()
					};
					ShaderConductor::Compiler::ResultDesc SpvTextResultDesc = ShaderConductor::Compiler::Disassemble(SpvDisassembleDesc);
					FString SpvSourceText = {(int32)SpvTextResultDesc.target.Size(), static_cast<const char*>(SpvTextResultDesc.target.Data()) };
					FFileHelper::SaveStringToFile(SpvSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".spvasm"));
#endif
				}
				else
				{
					InShader->SetCompilationResult(ShaderBlob);
#if DEBUG_SHADER
					TRefCountPtr<IDxcBlob> PdbBlob;
					TRefCountPtr<IDxcBlobUtf16> PdbNameBlob;
					if (SUCCEEDED(CompileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(PdbBlob.GetInitReference()), PdbNameBlob.GetInitReference())))
					{
						const FString PdbName = PdbNameBlob->GetStringPointer();
						const FString PdbFile = PathHelper::SavedShaderDir() / "Pdb" / PdbName;

						TArray<uint8> BinaryData{ (uint8*)PdbBlob->GetBufferPointer(), (int32)PdbBlob->GetBufferSize() };
						FFileHelper::SaveArrayToFile(BinaryData, *PdbFile);
					}
#endif
				}
			}
		}
		
		return IsCompilationSucceeded;
	 }
}
