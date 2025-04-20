#pragma once
#include "AssetObject/AssetObject.h"
#include "RenderResource/UniformBuffer.h"
#include "UI/Widgets/Property/PropertyData/PropertyData.h"
#include "RenderResource/Shader/Shader.h"

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
        
        void AddUniform(FString TypeName);
        void RefreshBuilder();
        
    private:
        TSharedRef<SWidget> GetCategoryMenu();
        bool HasBindingName(const FString& InName);
        TSharedPtr<FW::PropertyData> CreateUniformPropertyData(const FString& InTypeName, const FString& UniformMemberName, bool Enabled);
        
	public:
        bool bCurPsCompilationSucceed;
        FString SavedPixelShaderBody;
		FString PixelShaderBody;
        TRefCountPtr<FW::GpuShader> VertexShader, PixelShader;
        FSimpleDelegate OnRefreshBuilder;
        
        TSharedPtr<FW::PropertyCategory> BuiltInCategory;
        TSharedPtr<FW::PropertyCategory> CustomCategory;
        FW::UniformBufferBuilder CustomUniformBufferBuilder{FW::UniformBufferUsage::Persistant};
        FW::GpuBindGroupLayoutBuilder CustomBindGroupLayoutBuilder{ FW::BindingContext::PassSlot };
	};

}
