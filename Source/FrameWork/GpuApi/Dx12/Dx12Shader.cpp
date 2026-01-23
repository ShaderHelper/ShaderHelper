#include "CommonHeader.h"
#include "Dx12Shader.h"
#include "Common/Path/PathHelper.h"
#include "GpuApi/GpuFeature.h"
#include "ShaderConductor.hpp"
#include "GpuApi/GLSL.h"
#include "nir_spirv.h"
#include "nir_to_dxil.h"
extern "C" {
#include "dxil_spirv_nir.h"
}
#include "spirv_to_dxil.h"

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
		glsl_type_singleton_init_or_ref();
		DxCheck(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(Compiler.GetInitReference())));
		DxCheck(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(CompilerUitls.GetInitReference())));
	 }

	FString GetShaderProfile(ShaderType InStage, GpuShaderModel InModel)
	{
		FString ProfileName;
		if (InStage == ShaderType::VertexShader) {
			ProfileName += "vs";
		}
		else if (InStage == ShaderType::PixelShader) {
			ProfileName += "ps";
		}
		else if (InStage == ShaderType::ComputeShader) {
			ProfileName += "cs";
		}

		ProfileName += "_";
		ProfileName += FString::FromInt(InModel.Major);
		ProfileName += "_";
		ProfileName += FString::FromInt(InModel.Minor);

		return ProfileName;
	}

	mesa_shader_stage MapMesaStage(ShaderType InStage)
	{
		switch (InStage)
		{
		case FW::ShaderType::VertexShader:         return MESA_SHADER_VERTEX;
		case FW::ShaderType::PixelShader:          return MESA_SHADER_FRAGMENT;
		case FW::ShaderType::ComputeShader:        return MESA_SHADER_COMPUTE;
		default:
			AUX::Unreachable();
		}
	}

	static dxil_shader_model shader_model_d3d_to_dxil(D3D_SHADER_MODEL p_d3d_shader_model) {
		static_assert(SHADER_MODEL_6_0 == 0x60000);
		static_assert(SHADER_MODEL_6_3 == 0x60003);
		static_assert(D3D_SHADER_MODEL_6_0 == 0x60);
		static_assert(D3D_SHADER_MODEL_6_3 == 0x63);
		return (dxil_shader_model)((p_d3d_shader_model >> 4) * 0x10000 + (p_d3d_shader_model & 0xf));
	}

	static IDxcValidator*
		CreateDxcValidator(HMODULE dxil_mod)
	{
		DxcCreateInstanceProc dxil_create_func =
			(DxcCreateInstanceProc)(void*)GetProcAddress(dxil_mod, "DxcCreateInstance");
		if (!dxil_create_func) {
			return NULL;
		}

		IDxcValidator* dxc_validator;
		HRESULT hr = dxil_create_func(CLSID_DxcValidator,
			IID_PPV_ARGS(&dxc_validator));
		if (FAILED(hr)) {
			return NULL;
		}

		return dxc_validator;
	}

	bool Validate(IDxcValidator* val, void* data, size_t size)
	{
		if (!val)
			return false;

		TRefCountPtr<IDxcBlobEncoding> BlobEncoding;
		DxCheck(GShaderCompiler.CompilerUitls->CreateBlobFromPinned(data, size, CP_ACP, BlobEncoding.GetInitReference()));

		TRefCountPtr<IDxcOperationResult> result;
		val->Validate(BlobEncoding, DxcValidatorFlags_InPlaceEdit,
			result.GetInitReference());

		HRESULT hr;
		result->GetStatus(&hr);

		return SUCCEEDED(hr);
	}

	static bool ValidateDxil(struct blob* blob)
	{
		static HMODULE mod = LoadLibraryA("dxil.dll");
		static IDxcValidator* val = CreateDxcValidator(mod);

		bool res = Validate(val, blob->data,
			blob->size);
		return res;
	}

	static constexpr uint32_t REQUIRED_SHADER_MODEL = 0x62;

	bool ConvertSpirvToNir(const TArray<uint32>& InSpv, ShaderType InStage, const char* InEnrtyPoint, nir_shader** OutNir)
	{
		nir_shader_compiler_options compiler_options = {};
		const unsigned supported_bit_sizes = 16 | 32 | 64;
		dxil_get_nir_compiler_options(&compiler_options, shader_model_d3d_to_dxil(D3D_SHADER_MODEL(REQUIRED_SHADER_MODEL)), supported_bit_sizes, supported_bit_sizes);
		compiler_options.lower_base_vertex = false;

		*OutNir = spirv_to_nir(InSpv.GetData(), InSpv.Num(),
			nullptr, 0, MapMesaStage(InStage), InEnrtyPoint, dxil_spirv_nir_get_spirv_options(), &compiler_options);

		if (!*OutNir) {
			SH_LOG(LogShader, Error, TEXT("SPIR-V to NIR failed\n"));
			return false;
		}

		dxil_spirv_runtime_conf conf = {};
		conf.runtime_data_cbv.base_shader_register = 0;
		conf.runtime_data_cbv.register_space = 31;
		conf.push_constant_cbv.base_shader_register = 0;
		conf.push_constant_cbv.register_space = 30;
		conf.first_vertex_and_base_instance_mode = DXIL_SPIRV_SYSVAL_TYPE_ZERO;
		conf.shader_model_max = shader_model_d3d_to_dxil(D3D_SHADER_MODEL(REQUIRED_SHADER_MODEL));

		dxil_spirv_nir_prep(*OutNir);
		dxil_spirv_metadata dxil_metadata = {};
		dxil_spirv_nir_passes(*OutNir, &conf, &dxil_metadata);
		return true;
	}

	bool ConvertNirToDxil(nir_shader* Nir, TArray<uint8>& OutDxil)
	{
		nir_to_dxil_options nir_to_dxil_options = {};
		nir_to_dxil_options.environment = DXIL_ENVIRONMENT_VULKAN;
		nir_to_dxil_options.shader_model_max = shader_model_d3d_to_dxil(D3D_SHADER_MODEL(REQUIRED_SHADER_MODEL));
		nir_to_dxil_options.validator_version_max = NO_DXIL_VALIDATION;

		dxil_logger logger = {};
		logger.log = [](void* p_priv, const char* p_msg) {
#if DEBUG_SHADER
			SH_LOG(LogShader, Log, TEXT("DXIL Logger: %s"), UTF8_TO_TCHAR(p_msg));
#endif
		};

		blob dxil_blob = {};
		bool Success = nir_to_dxil(Nir, &nir_to_dxil_options, &logger, &dxil_blob);
		ralloc_free(Nir);
		if (!ValidateDxil(&dxil_blob))
		{
			SH_LOG(LogShader, Error, TEXT("Failed to validate DXIL\n"));
			blob_finish(&dxil_blob);
			return false;
		}
		OutDxil = { dxil_blob.data, (int)dxil_blob.size };
		blob_finish(&dxil_blob);

		return true;
	}

	bool ConvertSpirvToDxil(const TArray<uint32>& Spv, ShaderType InStage, const char* InEnrtyPoint, TArray<uint8>& OutDxil)
	{
		nir_shader* Nir = nullptr;
		if (!ConvertSpirvToNir(Spv, InStage, InEnrtyPoint, &Nir))
		{
			return false;
		}

		if (!ConvertNirToDxil(Nir, OutDxil))
		{
			return false;
		}
		return true;
	}

	 bool DxcCompiler::Compile(TRefCountPtr<Dx12Shader> InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs) const
	 {
		 FString ShaderName = InShader->GetShaderName();
		 FString HlslSource;
		 FString EntryPoint = InShader->GetEntryPoint();
		 if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::CompileFromSpriv))
		 {
			 if (InShader->SpvCode.IsEmpty())
			 {
				 return false;
			 }

			 TArray<uint8> Dxil;
			 if (!ConvertSpirvToDxil(InShader->SpvCode, InShader->GetShaderType(), TCHAR_TO_UTF8(*EntryPoint), Dxil))
			 {
				 return false;
			 }
			 IDxcBlobEncoding* ShaderBlob;
			 DxCheck(CompilerUitls->CreateBlob(Dxil.GetData(), Dxil.Num(), CP_ACP, &ShaderBlob));
			 InShader->SetCompilationResult(ShaderBlob);
			 return true;
		 }

		 if (InShader->GetShaderLanguage() == GpuShaderLanguage::GLSL)
		 {
#if DEBUG_SHADER
			 if (!ShaderName.IsEmpty())
			 {
				 FFileHelper::SaveStringToFile(InShader->GetProcessedSourceText(), *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".glsl"));
			 }
#endif
			 shaderc::SpvCompilationResult Result = CompileGlsl(InShader.GetReference(), ExtraArgs);

			 if(Result.GetCompilationStatus() != shaderc_compilation_status_success)
			 {
				 OutErrorInfo = UTF8_TO_TCHAR(Result.GetErrorMessage().c_str());
				 return false;
			 }
			 std::vector<uint32> Spv = { Result.cbegin(), Result.cend() };
			 TArray<uint32> SpvCode = { Result.cbegin(), (int32)(Result.cend() - Result.cbegin()) };
			 if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::GenSpvForDebugging))
			 {
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
				 if (!ShaderName.IsEmpty())
				 {
					 FFileHelper::SaveStringToFile(SpvSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".glsl" + ".spvasm"));
				 }
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
			 }
			 catch (const std::runtime_error& e)
			 {
				 OutErrorInfo = ANSI_TO_TCHAR(e.what());
				 return false;
			 }
		 }
		 else
		 {
			 HlslSource = InShader->GetProcessedSourceText();
		 }

			 /*TArray<uint8> Dxil;
			 if (!ConvertSpirvToDxil(SpvCode, InShader->GetShaderType(), TCHAR_TO_UTF8(*EntryPoint), Dxil))
			 {
				 return false;
			 }
			 IDxcBlobEncoding* ShaderBlob;
			 DxCheck(CompilerUitls->CreateBlob(Dxil.GetData(), Dxil.Num(), CP_ACP, &ShaderBlob));
			 InShader->SetCompilationResult(ShaderBlob);
			 return true;*/

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
					
					if (!ShaderName.IsEmpty())
					{
						FFileHelper::SaveStringToFile(SpvSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".hlsl" + ".spvasm"));
					}
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
