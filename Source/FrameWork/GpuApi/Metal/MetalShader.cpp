#include "CommonHeader.h"
#include "MetalShader.h"
#include "MetalDevice.h"
#include "ShaderConductor.hpp"
#include "Common/Path/PathHelper.h"
#include "GpuApi/Spirv/SpirvParser.h"

namespace FW
{
	MetalShader::MetalShader(const GpuShaderFileDesc& Desc)
		: GpuShader(Desc)
	{
		ProcessedSourceText = GpuShaderPreProcessor{ SourceText }
			.ReplacePrintStringLiteral()
			.Finalize();
	}

	MetalShader::MetalShader(const GpuShaderSourceDesc& Desc)
		: GpuShader(Desc)
	{
		ProcessedSourceText = GpuShaderPreProcessor{ SourceText }
			.ReplacePrintStringLiteral()
			.Finalize();
	}

    TRefCountPtr<MetalShader> CreateMetalShader(const GpuShaderFileDesc& FileDesc)
    {
        return new MetalShader(FileDesc);
    }

    TRefCountPtr<MetalShader> CreateMetalShader(const GpuShaderSourceDesc& SourceDesc)
    {
        return new MetalShader(SourceDesc);
    }

    bool CompileShaderFromMSL(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo)
    {
        check(!InShader->GetMslText().IsEmpty());
        
        NS::Error* err = nullptr;
        NS::SharedPtr<MTL::CompileOptions> CompOpt = NS::TransferPtr(MTL::CompileOptions::alloc()->init());
        CompOpt->setLanguageVersion(MTL::LanguageVersion2_2);
        
        MTLLibraryPtr ByteCodeLib = NS::TransferPtr(GDevice->newLibrary(FStringToNSString(InShader->GetMslText()), CompOpt.get(), &err));
        
        if(!ByteCodeLib) {
            OutErrorInfo = NSStringToFString(err->localizedDescription());
            SH_LOG(LogMetal, Error, TEXT("Msl compilation failed: %s"), *OutErrorInfo);
            return false;
        }
        
        MTLFunctionPtr ByteCodeFunc = NS::TransferPtr(ByteCodeLib->newFunction(FStringToNSString(InShader->GetEntryPoint())));
        if(!ByteCodeFunc) {
            SH_LOG(LogMetal, Error, TEXT("Msl compilation failed: EntryPoint not found: %s "), *InShader->GetEntryPoint());
            return false;
        }
        InShader->SetCompilationResult(MoveTemp(ByteCodeFunc));
        return true;
    }

    static ShaderConductor::ShaderStage MapShaderCunductorStage(ShaderType InType)
    {
        switch(InType)
        {
        case ShaderType::VertexShader:         return ShaderConductor::ShaderStage::VertexShader;
        case ShaderType::PixelShader:          return ShaderConductor::ShaderStage::PixelShader;
		case ShaderType::ComputeShader:        return ShaderConductor::ShaderStage::ComputeShader;
        default:
			AUX::Unreachable();
        }
    }

    bool CompileShaderFromHlsl(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo, const TArray<FString>& ExtraArgs)
{
		TArray<ShaderConductor::MacroDefine> Defines;
		Defines.Add({"FINAL_METAL"});
		
		ShaderConductor::Compiler::SourceDesc SourceDesc{};
		SourceDesc.stage = MapShaderCunductorStage(InShader->GetShaderType());
		auto ShaderNameUTF8 = StringCast<UTF8CHAR>(*InShader->GetShaderName());
		SourceDesc.fileName = (char*)ShaderNameUTF8.Get();
		
		//len + 1 for copying null terminator
		TArray<char> EntryPointAnsi{TCHAR_TO_ANSI(*InShader->GetEntryPoint()), InShader->GetEntryPoint().Len() + 1};
		SourceDesc.entryPoint = EntryPointAnsi.GetData();
		
		auto SourceUTF8 = StringCast<UTF8CHAR>(*InShader->GetProcessedSourceText());
		SourceDesc.source = (char*)SourceUTF8.Get();
		
		ShaderConductor::MacroDefine SpvMslOptions[] = {
			{"argument_buffers", "1"},
			{"enable_decoration_binding", "1"},
			{"force_active_argument_buffer_resources","1"}
		};
		
		ShaderConductor::Compiler::TargetDesc SpvTargetDesc{};
		SpvTargetDesc.language = ShaderConductor::ShadingLanguage::SpirV;
		
		ShaderConductor::Compiler::TargetDesc MslTargetDesc{};
		MslTargetDesc.language = ShaderConductor::ShadingLanguage::Msl_macOS;
		MslTargetDesc.version = "20200";
		MslTargetDesc.options = SpvMslOptions;
		MslTargetDesc.numOptions = UE_ARRAY_COUNT(SpvMslOptions);
		
		TArray<const char*> DxcArgs;
		DxcArgs.Add("-no-warnings");
		DxcArgs.Add("-fspv-preserve-bindings"); //For bindgroup-argumentbuffer
		//Storage buffers use the scalar layout instead of vector-relaxed 430
		//to make it consistent with structuredbuffer, so that user side can unify the struct
		DxcArgs.Add("-fvk-use-dx-layout");
		
		DxcArgs.Add("-HV");
		DxcArgs.Add("2021");
		
		if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::GenSpvForDebugging))
		{
			DxcArgs.Add("/Od");
			DxcArgs.Add("-fspv-debug=vulkan-with-source");
		}
		
		TArray<std::string> ExtraAnsiArgs;
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
					FFileHelper::LoadFileToString(ShaderText, *IncludedFile);
					ShaderText = GpuShaderPreProcessor{ ShaderText }
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
		
		ShaderConductor::Compiler::TargetDesc TargetDescs[2] = {SpvTargetDesc, MslTargetDesc};
		ShaderConductor::Compiler::ResultDesc Results[2];
		ShaderConductor::Compiler::Compile(SourceDesc, SCOptions, TargetDescs, 2, Results);
		if(Results[0].hasError)
		{
			FString ErrorInfo = static_cast<const char*>(Results[0].errorWarningMsg.Data());
			//SH_LOG(LogShader, Error, TEXT("Compilation failed: %s"), *ErrorInfo);
			OutErrorInfo = MoveTemp(ErrorInfo);
			return false;
		}
		else if(Results[1].hasError)
		{
			FString ErrorInfo = static_cast<const char*>(Results[1].errorWarningMsg.Data());
			//SH_LOG(LogShader, Error, TEXT("Compilation failed: %s"), *ErrorInfo);
			OutErrorInfo = MoveTemp(ErrorInfo);
			return false;
		}
		
		FString MslSourceText = {(int32)Results[1].target.Size(), static_cast<const char*>(Results[1].target.Data())};
#if DEBUG_SHADER
		ShaderConductor::Compiler::DisassembleDesc SpvasmDesc{ShaderConductor::ShadingLanguage::SpirV, static_cast<const uint8_t*>(Results[0].target.Data()), Results[0].target.Size()};
		ShaderConductor::Compiler::ResultDesc SpvasmResult = ShaderConductor::Compiler::Disassemble(SpvasmDesc);
		FString SpvSourceText = {(int32)SpvasmResult.target.Size(), static_cast<const char*>(SpvasmResult.target.Data())};
		
		FString ShaderName = InShader->GetShaderName();
		FFileHelper::SaveStringToFile(InShader->GetProcessedSourceText(), *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".hlsl"));
		FFileHelper::SaveStringToFile(SpvSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".spvasm"));
		FFileHelper::SaveStringToFile(MslSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".metal"));
#endif
		InShader->SetMslText(MoveTemp(MslSourceText));
		
		//Need the meta datas from spirv to abstract gpu api
		//For example, metal's threadgroupsize is not specified in the shader
		//and needs to be specified directly by dispatchThreadgroups
		TArray<uint32> SpvCode = {static_cast<const uint32*>(Results[0].target.Data()), Results[0].target.Size() / 4};
		SpvMetaContext MetaContext;
		SpvMetaVisitor MetaVisitor{MetaContext};
		SpirvParser Parser;
		Parser.Parse(SpvCode);
		Parser.Accept(&MetaVisitor);
		InShader->ThreadGroupSize = MetaContext.ThreadGroupSize;
		
		if (EnumHasAnyFlags(InShader->CompilerFlag, GpuShaderCompilerFlag::GenSpvForDebugging))
		{
			InShader->SpvCode = MoveTemp(SpvCode);
		}
        
        FString MslErrorInfo;
        bool IsSuccessfullyCompiledMsl = CompileShaderFromMSL(InShader, MslErrorInfo);
        
        if(!IsSuccessfullyCompiledMsl)
        {
            OutErrorInfo = MoveTemp(MslErrorInfo);
            return false;
        }
        
        return true;

    }
}
