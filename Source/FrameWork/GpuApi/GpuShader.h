#pragma once
#include "GpuResourceCommon.h"
#include <Misc/FileHelper.h>

namespace FW
{
	enum class GpuShaderFlag
	{
		Enable16bitType,
	};

	struct GpuShaderModel
	{
		uint8 Major = 6;
		uint8 Minor = 0;
	};

    class GpuShader : public GpuResource
    {
	public:
        //FromFile
		GpuShader(FString InFileName, ShaderType InType, const FString& ExtraDeclaration, FString InEntryPoint);
    
        //FromSource
		GpuShader(ShaderType InType, FString InSourceText, FString InShaderName, FString InEntryPoint);
        
        virtual bool IsCompiled() const {
            return false;
        }
        
        TOptional<FString> GetFileName() const { return FileName; }
        const TArray<FString>& GetIncludeDirs() const { return IncludeDirs; }
        
        const FString& GetShaderName() const { return ShaderName; }
        ShaderType GetShaderType() const {return Type;}
        const FString& GetSourceText() const { return SourceText; }
        const FString& GetEntryPoint() const { return EntryPoint; }
		GpuShaderModel GetShaderModelVer() const;

		void AddFlag(GpuShaderFlag InFlag) { Flags.Add(InFlag); }
		void AddFlags(TArray<GpuShaderFlag> InFlags) { Flags.Append(MoveTemp(InFlags)); }
		bool HasFlag(GpuShaderFlag InFlag) const { return Flags.Find(InFlag) != INDEX_NONE; }
        
    private:
        ShaderType Type;
        FString ShaderName;
        FString EntryPoint;
        //Hlsl
        FString SourceText;
        
        TOptional<FString> FileName;
        TArray<FString> IncludeDirs;

		TArray<GpuShaderFlag> Flags;
    };

	struct ShaderErrorInfo
	{
		int32 Row, Col;
		FString Info;
	};

	FRAMEWORK_API TArray<ShaderErrorInfo> ParseErrorInfoFromDxc(FStringView HlslErrorInfo);
}
