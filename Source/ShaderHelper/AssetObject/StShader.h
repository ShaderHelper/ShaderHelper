#pragma once
#include "AssetObject/ShaderAsset.h"
#include "RenderResource/UniformBuffer.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "RenderResource/Shader/Shader.h"

namespace SH
{
	class StShader : public ShaderAsset
	{
		REFLECTION_TYPE(StShader)
	public:
		StShader();
		~StShader();
        
		FW::GpuShader* GetPixelShader() const { return Shader; }
		static FW::GpuShader* GetVertexShader();
        static FW::UniformBuffer* GetBuiltInUb();
        static FW::UniformBufferBuilder& GetBuiltInUbBuilder();
        static FW::GpuBindGroupLayout* GetBuiltInBindLayout();
        static FW::GpuBindGroup* GetBuiltInBindGroup();
        static FW::GpuBindGroupLayoutBuilder& GetBuiltInBindLayoutBuilder();

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
		
        FString GetBinding() const;
        FString GetTemplateWithBinding() const;
        
		//ShObject interface
        TArray<TSharedRef<FW::PropertyData>>* GetPropertyDatas() override;
        TArray<TSharedRef<FW::PropertyData>> PropertyDatasFromBinding();
        TArray<TSharedRef<FW::PropertyData>> PropertyDatasFromUniform(const FW::UniformBufferBuilder& InBuilder, bool Enabled);
        
        void AddUniform(FString TypeName);
        void RefreshBuilder();
        
    private:
        TSharedRef<SWidget> GetCategoryMenu();
        bool HasBindingName(const FString& InName);
        TSharedPtr<FW::PropertyData> CreateUniformPropertyData(const FString& InTypeName, const FString& UniformMemberName, bool Enabled);
        
	public:
        FSimpleDelegate OnRefreshBuilder;
        
        TSharedPtr<FW::PropertyCategory> BuiltInCategory;
        TSharedPtr<FW::PropertyCategory> CustomCategory;
        FW::UniformBufferBuilder CustomUniformBufferBuilder{FW::UniformBufferUsage::Persistant};
        FW::GpuBindGroupLayoutBuilder CustomBindGroupLayoutBuilder{ FW::BindingContext::PassSlot };
	};

}
