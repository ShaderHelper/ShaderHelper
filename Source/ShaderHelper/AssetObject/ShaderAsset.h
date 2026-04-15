#pragma once
#include "AssetManager/AssetManager.h"
#include "GpuApi/GpuShader.h"

namespace SH
{
	struct ShaderDesc
	{
		FW::GpuShaderSourceDesc SourceDesc;
		TArray<FString> ExtraArgs;
	};

	class ShaderAsset : public FW::AssetObject
	{
		REFLECTION_TYPE(ShaderAsset)
	public:
		ShaderAsset();
		
		void Serialize(FArchive& Ar) override;
		
		//FullContent = EditorContent if no invisible content exists.
		virtual FString GetFullContent() const = 0;
		//The line num of invisible content(eg. binding codegen)
		virtual int32 GetExtraLineNum() const = 0;
		TArray<FString> GetIncludeDirs() const;
		FString GetShaderName() const;
		virtual ShaderDesc GetShaderDesc(const FString& InContent, FW::ShaderType InStage = FW::ShaderType::Vertex) const = 0;

		FW::AssetPtr<ShaderAsset> FindIncludeAsset(const FString& InShaderName);

		//Whether this shader asset type supports compilation (false for headers)
		virtual bool IsCompilable() const { return false; }
		//Compile the shader. Subclass implements its own compilation logic.
		virtual bool CompileShader(FString& OutError, FString& OutWarn) { return false; }
		//Query overall compilation success status
		virtual bool IsCompilationSucceeded() const { return false; }

		static FString LoadIncludeFile(const FString& IncludePath);

	protected:
		TArray<TSharedRef<FW::PropertyData>> BuildBindingPropertyDatas(FW::GpuShader* InShader) const;
		
	public:
		//Only visible content in editor
		FString EditorContent;
		FString SavedEditorContent;
		
		FSimpleMulticastDelegate OnShaderRefreshed;
	};

}
