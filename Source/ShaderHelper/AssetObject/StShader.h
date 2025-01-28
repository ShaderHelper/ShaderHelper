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
        
        static FW::UniformBuffer* GetBuiltInUb();
        static FW::UniformBufferBuilder& GetBuiltInUbBuilder();
        static FW::GpuBindGroupLayout* GetBuiltInBindLayout();
        static FW::GpuBindGroup* GetBuiltInBindGroup();
        static FW::GpuBindGroupLayoutBuilder& GetBuiltInBindLayoutBuilder();

	public:
		void Serialize(FArchive& Ar) override;
        void PostLoad() override;
		FString FileExtension() const override;
		const FSlateBrush* GetImage() const override;

        FString GetBinding() const;
        FString GetTemplateWithBinding() const;
		FString GetFullPs() const;
        
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
        FSimpleDelegate OnRefreshBuilder;
        
        //Custom
        TUniquePtr<FW::UniformBuffer> CustomUniformBuffer;
        TRefCountPtr<FW::GpuBindGroupLayout> CustomBindLayout;
        TRefCountPtr<FW::GpuBindGroup> CustomBindGroup;
        FW::UniformBufferBuilder CustomUniformBufferBuilder{FW::UniformBufferUsage::Persistant};
        FW::GpuBindGroupLayoutBuilder CustomBindGroupLayoutBuilder{1};
        
        TSharedPtr<FW::PropertyCategory> CustomCategory;
        //
	};

}
