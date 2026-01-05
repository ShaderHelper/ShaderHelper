#include "CommonHeader.h"
#include "Dx12Shader.h"
#include "Common/Path/PathHelper.h"
#include "GpuApi/GpuFeature.h"
#include "ShaderConductor.hpp"
#include "GpuApi/GLSL.h"

#include <Misc/FileHelper.h>
#include <stdexcept>
namespace FW
{
	Dx12Shader::Dx12Shader(const GpuShaderFileDesc& Desc)
		: GpuShader(Desc)
	{
		ProcessedSourceText = GpuShaderPreProcessor{ SourceText, ShaderLanguage }
			.ReplacePrintStringLiteral()
			.Finalize();
	}

	Dx12Shader::Dx12Shader(const GpuShaderSourceDesc& Desc)
		: GpuShader(Desc)
	{
		ProcessedSourceText = GpuShaderPreProcessor{ SourceText, ShaderLanguage }
			.ReplacePrintStringLiteral()
			.Finalize();
	}

	BindingType MapBindingType(D3D_SHADER_INPUT_TYPE Type)
	{
		switch (Type)
		{
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_CBUFFER:            return BindingType::UniformBuffer;
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_TEXTURE:			return BindingType::Texture;
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_SAMPLER:			return BindingType::Sampler;
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_STRUCTURED:			return BindingType::StructuredBuffer;
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWSTRUCTURED:	return BindingType::RWStructuredBuffer;
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_BYTEADDRESS:        return BindingType::RawBuffer;
		case D3D_SHADER_INPUT_TYPE::D3D_SIT_UAV_RWBYTEADDRESS:  return BindingType::RWRawBuffer;
		default:
			AUX::Unreachable();
		}
	}

	TArray<GpuShaderLayoutBinding> Dx12Shader::GetLayout() const
	{
		check(ByteCode.IsValid());
		TArray<GpuShaderLayoutBinding> ShaderLayoutBindings;
		TRefCountPtr<ID3D12ShaderReflection> Reflection;
		DxcBuffer DxilBuffer{.Ptr = ByteCode->GetBufferPointer(), .Size = ByteCode->GetBufferSize()};
		GShaderCompiler.CompilerUitls->CreateReflection(&DxilBuffer, IID_PPV_ARGS(Reflection.GetInitReference()));
		D3D12_SHADER_DESC ShaderDesc;
		Reflection->GetDesc(&ShaderDesc);
		for (uint32 Index = 0; Index < ShaderDesc.BoundResources; Index++)
		{
			D3D12_SHADER_INPUT_BIND_DESC BindDesc;
			Reflection->GetResourceBindingDesc(Index, &BindDesc);
			ShaderLayoutBindings.Add({
				.Name = BindDesc.Name,
				.Slot = (int)BindDesc.BindPoint,
				.Group = (int)BindDesc.Space,
				.Type = MapBindingType(BindDesc.Type)
			});

		}
		return ShaderLayoutBindings;
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
					ShaderText = GpuShaderPreProcessor{ ShaderText, Shader->GetShaderLanguage()}
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
		 FString ShaderName = InShader->GetShaderName();
		 FString HlslSource, EntryPoint;
		 if (InShader->GetShaderLanguage() == GpuShaderLanguage::GLSL)
		 {
#if DEBUG_SHADER
			 if (!ShaderName.IsEmpty())
			 {
				 FFileHelper::SaveStringToFile(InShader->GetProcessedSourceText(), *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".glsl"));
			 }
#endif
			 auto Result = CompileGlsl(InShader.GetReference(), ExtraArgs);

			 if(Result.GetCompilationStatus() != shaderc_compilation_status_success)
			 {
				 OutErrorInfo = UTF8_TO_TCHAR(Result.GetErrorMessage().c_str());
				 return false;
			 }

			 std::vector<uint32> Spv = { Result.cbegin(), Result.cend() };

			 if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::GenSpvForDebugging))
			 {
				 TArray<uint32> SpvCode = { Spv.data(), (int)Spv.size() };
#if DEBUG_SHADER
				 ShaderConductor::Compiler::DisassembleDesc SpvDisassembleDesc{
					 .language = ShaderConductor::ShadingLanguage::SpirV,
					 .binary = (uint8*)SpvCode.GetData(),
					 .binarySize = (uint32_t)SpvCode.Num() * 4,
				 };
				 ShaderConductor::Compiler::ResultDesc SpvTextResultDesc = ShaderConductor::Compiler::Disassemble(SpvDisassembleDesc);
				 FString SpvSourceText = { (int32)SpvTextResultDesc.target.Size(), static_cast<const char*>(SpvTextResultDesc.target.Data()) };
				 if (SpvTextResultDesc.hasError)
				 {
					 FString ErrorInfo = static_cast<const char*>(SpvTextResultDesc.errorWarningMsg.Data());
					 SpvSourceText = MoveTemp(ErrorInfo);
				 }
				 FFileHelper::SaveStringToFile(SpvSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".glsl" + ".spvasm"));
#endif
				 InShader->SpvCode = MoveTemp(SpvCode);
				 return true;
			 }

			 ShaderConductor::Compiler::TargetDesc HlslTargetDesc{};
			 HlslTargetDesc.language = ShaderConductor::ShadingLanguage::Hlsl;
			 HlslTargetDesc.version = "66";
			 try
			 {
				 ShaderConductor::Compiler::ResultDesc ShaderResultDesc = ShaderConductor::Compiler::SpvCompile({}, { Spv.data(), (uint32)Spv.size() * 4 }, "main",
					 MapShaderCunductorStage(InShader->GetShaderType()), HlslTargetDesc);
				 HlslSource = { (int32)ShaderResultDesc.target.Size(), static_cast<const char*>(ShaderResultDesc.target.Data()) };
				 EntryPoint = "main";
			 }
			 catch (const std::runtime_error& e)
			 {
				 OutErrorInfo =  ANSI_TO_TCHAR(e.what());
				 return false;
			 }
		 }
		 else
		 {
			 HlslSource = InShader->GetProcessedSourceText();
			 EntryPoint = InShader->GetEntryPoint();
		 }

#if DEBUG_SHADER
		 if (!ShaderName.IsEmpty())
		 {
			 FFileHelper::SaveStringToFile(HlslSource, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".hlsl"));
		 }
#endif

		TRefCountPtr<IDxcBlobEncoding> BlobEncoding;
		TRefCountPtr<IDxcResult> CompileResult;
		auto SourceText = StringCast<UTF8CHAR>(*HlslSource);
		DxCheck(CompilerUitls->CreateBlobFromPinned(SourceText.Get(), SourceText.Length() * sizeof(UTF8CHAR), CP_UTF8, BlobEncoding.GetInitReference()));

		TArray<const TCHAR*> Arguments;
		Arguments.Add(*ShaderName);
		Arguments.Add(TEXT("/Qstrip_debug"));
		Arguments.Add(TEXT("/E"));
		Arguments.Add(*EntryPoint);

		FString ShaderProfile = GetShaderProfile(InShader->GetShaderType(), InShader->GetShaderModelVer());
		Arguments.Add(TEXT("/T"));
		Arguments.Add(*ShaderProfile);
#if DEBUG_SHADER
		Arguments.Add(TEXT("/Zi"));
#endif
		Arguments.Add(TEXT("/Od")); //TODO
		Arguments.Add(TEXT("-Zpr"));
        //Arguments.Add(TEXT("-no-warnings"));

		if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::GenSpvForDebugging))
		{
			Arguments.Add(TEXT("/Od"));
			Arguments.Add(TEXT("-fcgl"));
			Arguments.Add(TEXT("-spirv"));
			Arguments.Add(TEXT("-Vd"));
			Arguments.Add(TEXT("-fvk-use-dx-layout"));
			Arguments.Add(TEXT("-fspv-debug=vulkan-with-source"));
			//Arguments.Add(TEXT("-fspv-reflect"));
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
					if (SpvTextResultDesc.hasError)
					{
						FString ErrorInfo = static_cast<const char*>(SpvTextResultDesc.errorWarningMsg.Data());
						SpvSourceText = MoveTemp(ErrorInfo);
					}
					FFileHelper::SaveStringToFile(SpvSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".hlsl" + ".spvasm"));
#endif
				}
				else
				{
					InShader->SetCompilationResult(ShaderBlob);
#if DEBUG_SHADER
					TRefCountPtr<IDxcBlob> PdbBlob;
					TRefCountPtr<IDxcBlobUtf16> PdbNameBlob;
					if (SUCCEEDED(CompileResult->GetOutput(DXC_OUT_PDB, IID_PPV_ARGS(PdbBlob.GetInitReference()), PdbNameBlob.GetInitReference()))
						&& PdbNameBlob)
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
