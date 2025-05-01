#include "CommonHeader.h"
#include "MetalShader.h"
#include "MetalDevice.h"
#include "ShaderConductor.hpp"
#include "Common/Path/PathHelper.h"

namespace FW
{
	MetalShader::MetalShader(const FString& InFileName, ShaderType InType, const FString& ExtraDeclaration, const FString& InEntryPoint)
		: GpuShader(InFileName, InType, ExtraDeclaration, InEntryPoint)
	{
		ProcessedSourceText = GpuShaderPreProcessor{ SourceText }
			.ReplaceTextToArray()
			.Finalize();
	}

	MetalShader::MetalShader(ShaderType InType, const FString& InSourceText, const FString& InShaderName, const FString& InEntryPoint)
		: GpuShader(InType, InSourceText, InShaderName, InEntryPoint)
	{
		ProcessedSourceText = GpuShaderPreProcessor{ SourceText }
			.ReplaceTextToArray()
			.Finalize();
	}

    TRefCountPtr<MetalShader> CreateMetalShader(ShaderType InType, const FString& InSourceText, const FString& ShaderName, const FString& InEntryPoint)
    {
        return new MetalShader(InType, InSourceText, ShaderName, InEntryPoint);
    }

    TRefCountPtr<MetalShader> CreateMetalShader(const FString& FileName, ShaderType InType, const FString& ExtraDeclaration, const FString& EntryPoint)
    {
        return new MetalShader(FileName, InType, ExtraDeclaration, EntryPoint);
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
        default:
			AUX::Unreachable();
        }
    }

    bool CompileShaderFromHlsl(TRefCountPtr<MetalShader> InShader, FString& OutErrorInfo)
    {
        TArray<ShaderConductor::MacroDefine> Defines;
        Defines.Add({"FINAL_METAL"});
        
        ShaderConductor::Compiler::SourceDesc SourceDesc{};
        SourceDesc.stage = MapShaderCunductorStage(InShader->GetShaderType());
        
        //len + 1 for copying null terminator
        TArray<char> EntryPointAnsi{TCHAR_TO_ANSI(*InShader->GetEntryPoint()), InShader->GetEntryPoint().Len() + 1};
        SourceDesc.entryPoint = EntryPointAnsi.GetData();
        
		auto SourceUTF8 = StringCast<UTF8CHAR>(*InShader->GetSourceText());
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

		DxcArgs.Add("-HV");
		DxcArgs.Add("2021");
        
        SourceDesc.loadIncludeCallback = [InShader](const char* includeName) -> ShaderConductor::Blob {
            for(const FString& IncludeDir : InShader->GetIncludeDirs())
            {
                TArray<uint8> IncludeBlob;
                FString IncludedFile = FPaths::Combine(IncludeDir, includeName);
                if(IFileManager::Get().FileExists(*IncludedFile))
                {
                    FFileHelper::LoadFileToArray(IncludeBlob, *IncludedFile);
                    return ShaderConductor::Blob(IncludeBlob.GetData(), IncludeBlob.Num());
                }
            }
            return {};
        };
        
        ShaderConductor::Compiler::Options SCOptions;
        SCOptions.DXCArgs = DxcArgs.GetData();
        SCOptions.numDXCArgs = DxcArgs.Num();
		GpuShaderModel Sm = InShader->GetShaderModelVer();
		SCOptions.shaderModel = {Sm.Major, Sm.Minor};

		if (InShader->HasFlag(GpuShaderFlag::Enable16bitType))
		{
            Defines.Add({"ENABLE_16BIT_TYPE"});
			SCOptions.enable16bitTypes = true;
		}
        
        SourceDesc.defines = Defines.GetData();
        SourceDesc.numDefines = Defines.Num();
        
        ShaderConductor::Compiler::TargetDesc TargetDescs[2] = {SpvTargetDesc, MslTargetDesc};
        ShaderConductor::Compiler::ResultDesc Results[2];
        ShaderConductor::Compiler::Compile(SourceDesc, SCOptions, TargetDescs, 2, Results);
        if(Results[1].errorWarningMsg.Size() > 0)
        {
            FString ErrorInfo = static_cast<const char*>(Results[1].errorWarningMsg.Data());
            SH_LOG(LogMetal, Error, TEXT("Hlsl compilation failed: %s"), *ErrorInfo);
			OutErrorInfo = MoveTemp(ErrorInfo);
            return false;
        }
        
        FString MslSourceText = {(int32)Results[1].target.Size(), static_cast<const char*>(Results[1].target.Data())};
#if DEBUG_SHADER
        ShaderConductor::Compiler::DisassembleDesc SpvasmDesc{ShaderConductor::ShadingLanguage::SpirV, static_cast<const uint8_t*>(Results[0].target.Data()), Results[0].target.Size()};
        ShaderConductor::Compiler::ResultDesc SpvasmResult = ShaderConductor::Compiler::Disassemble(SpvasmDesc);
        FString SpvSourceText = {(int32)SpvasmResult.target.Size(), static_cast<const char*>(SpvasmResult.target.Data())};
        
        FString ShaderName = InShader->GetShaderName();
        FFileHelper::SaveStringToFile(InShader->GetSourceText(), *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".hlsl"));
        FFileHelper::SaveStringToFile(SpvSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".spvasm"));
        FFileHelper::SaveStringToFile(MslSourceText, *(PathHelper::SavedShaderDir() / ShaderName / ShaderName + ".metal"));
#endif
        InShader->SetMslText(MoveTemp(MslSourceText));
        
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
