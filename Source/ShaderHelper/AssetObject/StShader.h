#pragma once
#include "AssetObject/ShaderAsset.h"
#include "RenderResource/UniformBuffer.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "RenderResource/Shader/Shader.h"

namespace SH
{
	enum class ShaderToySlotType
	{
		Texture2D,
		TextureCube
	};

	class StShader : public ShaderAsset
	{
		REFLECTION_TYPE(StShader)
	public:
		StShader();
		~StShader();
        
		FW::GpuShader* GetPixelShader() const { return Shader; }
		FW::GpuShader* GetVertexShader();
        static FW::UniformBufferBuilder& GetBuiltInUbBuilder();
		TRefCountPtr<FW::GpuBindGroupLayout> GetBuiltInBindLayout(uint8 CubeChannelMask = 0);
        FW::GpuBindGroupLayoutBuilder GetBuiltInBindLayoutBuilder(uint8 CubeChannelMask) const;

	public:
		//AssetObject interface
		void Serialize(FArchive& Ar) override;
        void PostLoad() override;
		FString FileExtension() const override;
		const FSlateBrush* GetImage() const override;

		//ShaderAsset interface
		FString GetFullContent() const override;
		int32 GetExtraLineNum() const override;
		FW::GpuShaderSourceDesc GetShaderDesc(const FString& InContent) const override;
		int32 GetExtraLineNum(uint8 CubeChannelMask) const;
		FW::GpuShaderSourceDesc GetShaderDesc(const FString& InContent, uint8 CubeChannelMask) const;
		
        FString GetBinding(uint8 CubeChannelMask = 0) const;
        FString GetTemplateWithBinding(uint8 CubeChannelMask = 0) const;
        
		//ShObject interface
		bool CanChangeProperty(FW::PropertyData* InProperty) override;
		void PostPropertyChanged(FW::PropertyData* InProperty) override;
        TArray<TSharedRef<FW::PropertyData>>* GetPropertyDatas() override;
        TArray<TSharedRef<FW::PropertyData>> PropertyDatasFromBinding();
        TArray<TSharedRef<FW::PropertyData>> PropertyDatasFromUniform(const FW::UniformBufferBuilder& InBuilder, bool Enabled);
        
        void AddUniform(TAttribute<FText> TypeName);
        void RefreshBuilder();
    private:
        TSharedRef<SWidget> GetCategoryMenu();
        bool HasBindingName(const FString& InName);
        TSharedPtr<FW::PropertyData> CreateUniformPropertyData(const TAttribute<FText>& InTypeName, const FString& UniformMemberName, bool Enabled);
        
	public:
		FW::GpuShaderLanguage Language;
		uint8 CubeChannelMask = 0;

        TSharedPtr<FW::PropertyCategory> BuiltInCategory;
        TSharedPtr<FW::PropertyCategory> CustomCategory;
        FW::UniformBufferBuilder CustomUniformBufferBuilder{FW::UniformBufferUsage::Persistant};
        FW::GpuBindGroupLayoutBuilder CustomBindGroupLayoutBuilder{ FW::BindingContext::PassSlot };
	};

}
