#include "CommonHeader.h"
#include "MetalShader.h"
#include "MetalDevice.h"
#include "ShaderConductor.hpp"
#include "Common/Path/PathHelper.h"
#include "GpuApi/Spirv/SpirvParser.h"
#include "GpuApi/GLSL.h"

namespace FW
{
	MetalShader::MetalShader(const GpuShaderFileDesc& Desc)
		: GpuShader(Desc)
	{
	}

	MetalShader::MetalShader(const GpuShaderSourceDesc& Desc)
		: GpuShader(Desc)
	{
	}

    bool CompileShaderFromMSL(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo)
    {
        check(!InShader->GetMslText().IsEmpty());
        
        NS::Error* err = nullptr;
        NS::SharedPtr<MTL::CompileOptions> CompOpt = NS::TransferPtr(MTL::CompileOptions::alloc()->init());
        CompOpt->setLanguageVersion(MTL::LanguageVersion2_2);
		CompOpt->setOptimizationLevel(MTL::LibraryOptimizationLevel::LibraryOptimizationLevelSize);
        
        MTLLibraryPtr ByteCodeLib = NS::TransferPtr(GDevice->newLibrary(FStringToNSString(InShader->GetMslText()), CompOpt.get(), &err));
        
        if(!ByteCodeLib) {
            OutErrorInfo = NSStringToFString(err->localizedDescription());
            SH_LOG(LogMetal, Error, TEXT("Msl compilation failed: %s"), *OutErrorInfo);
            return false;
        }
		
		FString EntryPoint = InShader->GetEntryPoint();
		//Msl can not call a metal function main
		if(EntryPoint == "main")
		{
			//spirv-cross default name
			EntryPoint = "main0";
		}
        MTLFunctionPtr ByteCodeFunc = NS::TransferPtr(ByteCodeLib->newFunction(FStringToNSString(EntryPoint)));
        if(!ByteCodeFunc) {
            SH_LOG(LogMetal, Error, TEXT("Msl compilation failed: EntryPoint not found: %s "), *EntryPoint);
            return false;
        }
        InShader->SetCompilationResult(MoveTemp(ByteCodeFunc));
        return true;
    }

    bool CompileMetalShader(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo, FString& OutWarnInfo, const TArray<FString>& ExtraArgs)
	{
		FString ShaderName = InShader->GetShaderName();
		FString EntryPoint = InShader->GetEntryPoint();;
		TArray<uint32> SpvCode;

		if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::CompileFromSpvCode))
		{
			SpvCode = MoveTemp(InShader->SpvCode);
		}
		else if(InShader->GetShaderLanguage() == GpuShaderLanguage::GLSL)
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
			Defines.Add({"FINAL_METAL"});
			
			ShaderConductor::Compiler::SourceDesc SourceDesc{};
			SourceDesc.stage = MapShaderCunductorStage(InShader->GetShaderType());
			auto ShaderNameUTF8 = StringCast<UTF8CHAR>(*ShaderName);
			SourceDesc.fileName = (char*)ShaderNameUTF8.Get();
			
			//len + 1 for copying null terminator
			TArray<char> EntryPointAnsi{TCHAR_TO_ANSI(*EntryPoint), EntryPoint.Len() + 1};
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
			std::string ShiftB, ShiftT, ShiftS, ShiftU;
			if (!EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::SkipBindingShift))
			{
				DxcArgs.Add("-fvk-auto-shift-bindings");
				// Shift HLSL register types into separate argument buffer index ranges to avoid collisions.
				ShiftB = std::to_string(BindingShift_Buffer);
				ShiftT = std::to_string(BindingShift_Texture);
				ShiftS = std::to_string(BindingShift_Sampler);
				ShiftU = std::to_string(BindingShift_UAV);
				DxcArgs.Add("-fvk-b-shift"); DxcArgs.Add(ShiftB.c_str()); DxcArgs.Add("all");
				DxcArgs.Add("-fvk-t-shift"); DxcArgs.Add(ShiftT.c_str()); DxcArgs.Add("all");
				DxcArgs.Add("-fvk-s-shift"); DxcArgs.Add(ShiftS.c_str()); DxcArgs.Add("all");
				DxcArgs.Add("-fvk-u-shift"); DxcArgs.Add(ShiftU.c_str()); DxcArgs.Add("all");
			}
			//DxcArgs.Add("-Gis");
			DxcArgs.Add("/Od"); //TODO
			
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
				for(const FString& IncludeDir : InShader->GetIncludeDirs())
				{
					FString IncludedFile = FPaths::Combine(IncludeDir, includeName);
					if(IFileManager::Get().FileExists(*IncludedFile))
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
						ShaderText = GpuShaderPreProcessor{ ShaderText, InShader->GetShaderLanguage()}
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
			SCOptions.shaderModel = {Sm.Major, Sm.Minor};
			
			if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::Enable16bitType))
			{
				Defines.Add({"ENABLE_16BIT_TYPE"});
				SCOptions.enable16bitTypes = true;
			}
			
			SourceDesc.defines = Defines.GetData();
			SourceDesc.numDefines = Defines.Num();
			
			ShaderConductor::Compiler::Compile(SourceDesc, SCOptions, &SpvTargetDesc, 1, &SpvResult);
			if(SpvResult.hasError)
			{
				FString ErrorInfo = static_cast<const char*>(SpvResult.errorWarningMsg.Data());
				//SH_LOG(LogShader, Error, TEXT("Compilation failed: %s"), *ErrorInfo);
				OutErrorInfo = MoveTemp(ErrorInfo);
				return false;
			}
			else
			{
				OutWarnInfo = static_cast<const char*>(SpvResult.errorWarningMsg.Data());
			}
			
#if DEBUG_SHADER
			ShaderConductor::Compiler::DisassembleDesc SpvasmDesc{ShaderConductor::ShadingLanguage::SpirV, static_cast<const uint8_t*>(SpvResult.target.Data()), SpvResult.target.Size()};
			ShaderConductor::Compiler::ResultDesc SpvasmResult = ShaderConductor::Compiler::Disassemble(SpvasmDesc);
			FString SpvSourceText = {(int32)SpvasmResult.target.Size(), static_cast<const char*>(SpvasmResult.target.Data())};
			if (!ShaderName.IsEmpty())
			{
				FFileHelper::SaveStringToFile(SpvSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".hlsl" + ".spvasm"));
			}
#endif
			SpvCode = { static_cast<const uint32*>(SpvResult.target.Data()), (int)SpvResult.target.Size() / 4 };
		}
		
		if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::GenSpvForDebugging))
		{
			InShader->SpvCode = MoveTemp(SpvCode);
			return true;
		}

		//Need the meta datas from spirv to abstract gpu api
		//For example, metal's threadgroupsize is not specified in the shader
		//and needs to be specified directly by dispatchThreadgroups
		SpvMetaContext MetaContext;
		SpvMetaVisitor MetaVisitor{ MetaContext };
		SpirvParser Parser;
		Parser.Parse(SpvCode);
		Parser.Accept(&MetaVisitor);
		InShader->ThreadGroupSize = MetaContext.ThreadGroupSize;
		
		ShaderConductor::MacroDefine SpvMslOptions[] = {
			{"argument_buffers", "1"},
			{"enable_decoration_binding", "1"},
			{"force_active_argument_buffer_resources","1"}
		};
		ShaderConductor::Compiler::TargetDesc MslTargetDesc{};
		MslTargetDesc.language = ShaderConductor::ShadingLanguage::Msl_macOS;
		MslTargetDesc.version = "20200";
		MslTargetDesc.options = SpvMslOptions;
		MslTargetDesc.numOptions = UE_ARRAY_COUNT(SpvMslOptions);
		
		ShaderConductor::Compiler::ResultDesc MslResult;
		
		try
		{
			auto EntryPointUTF8 = StringCast<UTF8CHAR>(*EntryPoint);
			MslResult = ShaderConductor::Compiler::SpvCompile({}, { SpvCode.GetData(), (uint32)SpvCode.Num() * 4 }, (const char*)EntryPointUTF8.Get(),
				MapShaderCunductorStage(InShader->GetShaderType()), MslTargetDesc);
			InShader->SpvCode = MoveTemp(SpvCode);
		}
		catch (const std::runtime_error& e)
		{
			OutErrorInfo =  ANSI_TO_TCHAR(e.what());
			return false;
		}

		if (MslResult.hasError)
		{
			FString ErrorInfo = static_cast<const char*>(MslResult.errorWarningMsg.Data());
			//SH_LOG(LogShader, Error, TEXT("Compilation failed: %s"), *ErrorInfo);
			OutErrorInfo = MoveTemp(ErrorInfo);
			return false;

		}

		FString MslSourceText = { (int32)MslResult.target.Size(), static_cast<const char*>(MslResult.target.Data()) };
#if DEBUG_SHADER
		if (!ShaderName.IsEmpty())
		{
			FFileHelper::SaveStringToFile(MslSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".metal"));
		}
#endif
		InShader->SetMslText(MoveTemp(MslSourceText));

		FString MslErrorInfo;
		bool IsSuccessfullyCompiledMsl = CompileShaderFromMSL(InShader, MslErrorInfo);

		if (!IsSuccessfullyCompiledMsl)
		{
			OutErrorInfo = MoveTemp(MslErrorInfo);
			return false;
		}
       
        return true;

    }
}
