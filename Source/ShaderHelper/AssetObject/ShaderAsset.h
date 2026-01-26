#pragma once
#include "AssetObject/AssetObject.h"
#include "GpuApi/GpuShader.h"

namespace SH
{
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
		virtual FW::GpuShaderSourceDesc GetShaderDesc(const FString& InContent) const = 0;
		
	public:
		//Only visible content in editor
		FString EditorContent;
		FString SavedEditorContent;
		TRefCountPtr<FW::GpuShader> Shader;
		bool bCompilationSucceed{};
		
		FSimpleMulticastDelegate OnRefreshBuilder;
	};

}
