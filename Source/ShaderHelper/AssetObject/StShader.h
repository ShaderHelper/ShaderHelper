#pragma once
#include "AssetObject/AssetObject.h"
#include "RenderResource/UniformBuffer.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"


namespace SH
{
	class StShader : public FW::AssetObject
	{
		REFLECTION_TYPE(StShader)
	public:
		StShader();
		~StShader();
        
        static FW::UniformBufferBuilder& GetBuiltInUbBuilder();
        static FW::GpuBindGroupLayoutBuilder& GetBuiltInBindingLayoutBuilder();

	public:
		void Serialize(FArchive& Ar) override;
        void PostLoad() override;
		FString FileExtension() const override;
		const FSlateBrush* GetImage() const override;

        FString GetBinding() const;
        FString GetTemplateWithBinding() const;
		FString GetFullShader() const;
        
        TArray<TSharedRef<FW::PropertyData>>* GetPropertyDatas() override;
        TArray<TSharedRef<FW::PropertyData>> PropertyDatasFromBinding();
        TArray<TSharedRef<FW::PropertyData>> PropertyDatasFromUniform(const FW::UniformBufferBuilder& InBuilder, bool Enabled);
        
        template<typename UniformType>
        void AddUniform();
        void RefreshBuilder();
        
    private:
        TSharedRef<SWidget> GetCategoryMenu();
        
	public:
		FString PixelShaderBody;
        TRefCountPtr<FW::GpuShader> VertexShader, PixelShader;
        
        //Custom
        FW::UniformBufferBuilder CustomUniformBufferBuilder{FW::UniformBufferUsage::Persistant};
        FW::GpuBindGroupLayoutBuilder CustomBindGroupLayoutBuilder{1};
        
        TSharedPtr<FW::PropertyCategory> CustomCategory;
        //
	};

}
