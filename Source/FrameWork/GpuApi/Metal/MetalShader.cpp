#include "CommonHeader.h"
#include "MetalShader.h"
#include "MetalDevice.h"
#include "ShaderConductor.hpp"
#include "Common/Path/PathHelper.h"

namespace FW
{
    TRefCountPtr<MetalShader> CreateMetalShader(ShaderType InType, FString InSourceText, FString ShaderName, FString InEntryPoint)
    {
        return new MetalShader(InType, MoveTemp(InSourceText), MoveTemp(ShaderName), MoveTemp(InEntryPoint));
    }

    TRefCountPtr<MetalShader> CreateMetalShader(FString FileName, ShaderType InType, FString ExtraDeclaration, FString EntryPoint)
    {
        return new MetalShader(MoveTemp(FileName), InType, MoveTemp(ExtraDeclaration), MoveTemp(EntryPoint));
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
        
        TArray<char> SourceAnsi{TCHAR_TO_ANSI(*InShader->GetSourceText()), InShader->GetSourceText().Len() + 1};
        SourceDesc.source = SourceAnsi.GetData();
        
        ShaderConductor::MacroDefine SpvMslOptions[] = {
            {"argument_buffers", "1"},
            {"enable_decoration_binding", "1"},
            {"force_active_argument_buffer_resources","1"}
        };
        
        ShaderConductor::Compiler::TargetDesc TargetDesc{};
        TargetDesc.language = ShaderConductor::ShadingLanguage::Msl_macOS;
		TargetDesc.version = "20200";
        TargetDesc.options = SpvMslOptions;
        TargetDesc.numOptions = UE_ARRAY_COUNT(SpvMslOptions);
        
        TArray<const char*> DxcArgs;
        DxcArgs.Add("-fspv-preserve-bindings");
        
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
        const ShaderConductor::Compiler::ResultDesc Result = ShaderConductor::Compiler::Compile(SourceDesc, SCOptions, TargetDesc);
        if(Result.errorWarningMsg.Size() > 0)
        {
            FString ErrorInfo = static_cast<const char*>(Result.errorWarningMsg.Data());
            SH_LOG(LogMetal, Error, TEXT("Hlsl compilation failed: %s"), *ErrorInfo);
			OutErrorInfo = MoveTemp(ErrorInfo);
            return false;
        }
        
        FString MslSourceText = {static_cast<const char*>(Result.target.Data()), (int32)Result.target.Size()};
#if DEBUG_SHADER
        FFileHelper::SaveStringToFile(MslSourceText, *(PathHelper::SavedShaderDir() / InShader->GetShaderName() + ".metal"));
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
