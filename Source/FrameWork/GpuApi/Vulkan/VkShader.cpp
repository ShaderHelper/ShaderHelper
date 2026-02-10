#include "CommonHeader.h"
#include "VkShader.h"
#include "ShaderConductor.hpp"
#include "GpuApi/GLSL.h"

namespace FW
{
	VulkanShader::VulkanShader(const GpuShaderFileDesc& Desc) : GpuShader(Desc)
	{
		ProcessedSourceText = GpuShaderPreProcessor{ SourceText, ShaderLanguage }
			.ReplacePrintStringLiteral()
			.Finalize();
	}

	VulkanShader::VulkanShader(const GpuShaderSourceDesc& Desc) : GpuShader(Desc)
	{
		ProcessedSourceText = GpuShaderPreProcessor{ SourceText, ShaderLanguage }
			.ReplacePrintStringLiteral()
			.Finalize();
	}

	TArray<GpuShaderLayoutBinding> VulkanShader::GetLayout() const
	{
		return TArray<GpuShaderLayoutBinding>();
	}

	bool CompileVulkanShader(TRefCountPtr<VulkanShader> InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs)
	{
		FString ShaderName = InShader->GetShaderName();
		FString EntryPoint = InShader->GetEntryPoint();;
		TArray<uint32> SpvCode;

		if (InShader->GetShaderLanguage() == GpuShaderLanguage::GLSL)
		{
#if DEBUG_SHADER
			if (!ShaderName.IsEmpty())
			{
				FFileHelper::SaveStringToFile(InShader->GetProcessedSourceText(), *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".glsl"));
			}
#endif
			auto Result = CompileGlsl(InShader.GetReference(), ExtraArgs);

			if (Result.GetCompilationStatus() != shaderc_compilation_status_success)
			{
				OutErrorInfo = UTF8_TO_TCHAR(Result.GetErrorMessage().c_str());
				return false;
			}

			std::vector<uint32> Spv = { Result.cbegin(), Result.cend() };
			SpvCode = { Spv.data(), (int)Spv.size() };
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
		}
		else
		{
#if DEBUG_SHADER
			if (!ShaderName.IsEmpty())
			{
				FFileHelper::SaveStringToFile(InShader->GetProcessedSourceText(), *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".hlsl"));
			}
#endif
			TArray<ShaderConductor::MacroDefine> Defines;
			Defines.Add({ "FINAL_SPIRV" });

			ShaderConductor::Compiler::SourceDesc SourceDesc{};
			SourceDesc.stage = MapShaderCunductorStage(InShader->GetShaderType());
			auto ShaderNameUTF8 = StringCast<UTF8CHAR>(*ShaderName);
			SourceDesc.fileName = (char*)ShaderNameUTF8.Get();

			//len + 1 for copying null terminator
			TArray<char> EntryPointAnsi{ TCHAR_TO_ANSI(*EntryPoint), EntryPoint.Len() + 1 };
			SourceDesc.entryPoint = EntryPointAnsi.GetData();

			auto SourceUTF8 = StringCast<UTF8CHAR>(*InShader->GetProcessedSourceText());
			SourceDesc.source = (char*)SourceUTF8.Get();

			ShaderConductor::Compiler::TargetDesc SpvTargetDesc{};
			SpvTargetDesc.language = ShaderConductor::ShadingLanguage::SpirV;

			ShaderConductor::Compiler::ResultDesc SpvResult;

			TArray<const char*> DxcArgs;
			//DxcArgs.Add("-no-warnings");
			DxcArgs.Add("-fspv-preserve-bindings"); //For bindgroup-argumentbuffer
			//Storage buffers use the scalar layout instead of vector-relaxed 430
			//to make it consistent with structuredbuffer, so that user side can unify the struct
			DxcArgs.Add("-fvk-use-dx-layout");
			//DxcArgs.Add("-Gis");
			DxcArgs.Add("/Od"); //TODO
			if (InShader->GetShaderType() == ShaderType::VertexShader)
			{
				DxcArgs.Add("-fvk-invert-y");
			}

			if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::GenSpvForDebugging))
			{
				DxcArgs.Add("/Od");
				DxcArgs.Add("-fcgl");
				DxcArgs.Add("-Vd");
				DxcArgs.Add("-fspv-debug=vulkan-with-source");
			}

			TArray<std::string> ExtraAnsiArgs;
			ExtraAnsiArgs.Reserve(ExtraArgs.Num());
			for (const FString& ExtraArg : ExtraArgs)
			{
				ExtraAnsiArgs.Add(TCHAR_TO_ANSI(*ExtraArg));
				DxcArgs.Add(ExtraAnsiArgs.Last().data());
			}

			SourceDesc.loadIncludeCallback = [InShader](const char* includeName) -> ShaderConductor::Blob {
				for (const FString& IncludeDir : InShader->GetIncludeDirs())
				{
					FString IncludedFile = FPaths::Combine(IncludeDir, includeName);
					if (IFileManager::Get().FileExists(*IncludedFile))
					{
						FString ShaderText;
						if (InShader->GetIncludeHandler())
						{
							ShaderText = InShader->GetIncludeHandler()(IncludedFile);
						}
						else
						{
							FFileHelper::LoadFileToString(ShaderText, *IncludedFile);
						}
						ShaderText = GpuShaderPreProcessor{ ShaderText, InShader->GetShaderLanguage() }
							.ReplacePrintStringLiteral()
							.Finalize();
						auto SourceText = StringCast<UTF8CHAR>(*ShaderText);
						return ShaderConductor::Blob(SourceText.Get(), SourceText.Length() * sizeof(UTF8CHAR));
					}
				}
				return {};
			};

			ShaderConductor::Compiler::Options SCOptions;
			SCOptions.DXCArgs = DxcArgs.GetData();
			SCOptions.numDXCArgs = DxcArgs.Num();
			GpuShaderModel Sm = InShader->GetShaderModelVer();
			SCOptions.shaderModel = { Sm.Major, Sm.Minor };

			if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::Enable16bitType))
			{
				Defines.Add({ "ENABLE_16BIT_TYPE" });
				SCOptions.enable16bitTypes = true;
			}

			SourceDesc.defines = Defines.GetData();
			SourceDesc.numDefines = Defines.Num();

			ShaderConductor::Compiler::Compile(SourceDesc, SCOptions, &SpvTargetDesc, 1, &SpvResult);
			if (SpvResult.hasError)
			{
				FString ErrorInfo = static_cast<const char*>(SpvResult.errorWarningMsg.Data());
				OutErrorInfo = MoveTemp(ErrorInfo);
				return false;
			}
			else
			{
				OutWarnInfo = static_cast<const char*>(SpvResult.errorWarningMsg.Data());
			}

#if DEBUG_SHADER
			ShaderConductor::Compiler::DisassembleDesc SpvasmDesc{ ShaderConductor::ShadingLanguage::SpirV, static_cast<const uint8_t*>(SpvResult.target.Data()), SpvResult.target.Size() };
			ShaderConductor::Compiler::ResultDesc SpvasmResult = ShaderConductor::Compiler::Disassemble(SpvasmDesc);
			FString SpvSourceText = { (int32)SpvasmResult.target.Size(), static_cast<const char*>(SpvasmResult.target.Data()) };
			if (!ShaderName.IsEmpty())
			{
				FFileHelper::SaveStringToFile(SpvSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".hlsl" + ".spvasm"));
			}
#endif
			SpvCode = { static_cast<const uint32*>(SpvResult.target.Data()), (int)SpvResult.target.Size() / 4 };
		}

		VkShaderModuleCreateInfo ModuleCreateInfo{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = (size_t)SpvCode.Num() * 4,
			.pCode = SpvCode.GetData()
		};
		VkShaderModule ShaderModule;
		VkCheck(vkCreateShaderModule(GDevice, &ModuleCreateInfo, nullptr, &ShaderModule));
		InShader->SetCompilationResult(ShaderModule);
		InShader->SpvCode = MoveTemp(SpvCode);
		return true;
	}
}
