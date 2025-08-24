#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/StShader.h"
#include "AssetManager/AssetManager.h"
#include "Editor/PreviewViewPort.h"
#include "AssetObject/DebuggableObject.h"
#include "AssetObject/ShaderToy/ShaderToy.h"

namespace SH
{
    class ShaderToyPassNodeOp : public FW::ShObjectOp
    {
        REFLECTION_TYPE(ShaderToyPassNodeOp)
    public:
        ShaderToyPassNodeOp() = default;
        
        FW::MetaType* SupportType() override;
		void OnCancelSelect(FW::ShObject* InObject) override;
        void OnSelect(FW::ShObject* InObject) override;
    };

	enum class ShaderToyFilterMode
	{
		Linear = (int)FW::SamplerFilter::Bilinear,
		Nearest = (int)FW::SamplerFilter::Point
	};

	enum class ShaderToyWrapMode
	{
		Clamp = (int)FW::SamplerAddressMode::Clamp,
		Repeat = (int)FW::SamplerAddressMode::Wrap
	};

	struct ShaderToyChannelDesc
	{
		ShaderToyFilterMode Filter = ShaderToyFilterMode::Linear;
		ShaderToyWrapMode Wrap = ShaderToyWrapMode::Repeat;
		friend FArchive& operator<<(FArchive& Ar, ShaderToyChannelDesc& ChannelDesc)
		{
			Ar << ChannelDesc.Filter;
			Ar << ChannelDesc.Wrap;
			return Ar;
		}
	};

	class ShaderToyPassNode : public FW::GraphNode, public DebuggableObject
	{
		REFLECTION_TYPE(ShaderToyPassNode)
	public:
		ShaderToyPassNode();
		ShaderToyPassNode(FW::AssetPtr<StShader> InShader);
		~ShaderToyPassNode();

	public:
        void InitPins() override;
		void Serialize(FArchive& Ar) override;
        void PostLoad() override;
		TSharedPtr<SWidget> ExtraNodeWidget() override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 0.27f, 0.13f, 0.0f }; }
        FW::ExecRet Exec(FW::GraphExecContext& Context) override;

		bool CanChangeProperty(FW::PropertyData* InProperty) override;
        void PostPropertyChanged(FW::PropertyData* InProperty) override;
        TArray<TSharedRef<FW::PropertyData>>* GetPropertyDatas() override;
    
		//ShDebuggableObject
		TRefCountPtr<FW::GpuTexture> OnStartDebugging() override;
		void OnFinalizePixel(const FW::Vector2u& PixelCoord) override;
		void OnEndDebuggging() override;
		ShaderAsset* GetShaderAsset() const;
		
    private:
		TRefCountPtr<FW::GpuBindGroup> GetBuiltInBindGroup();
		void InitShader();
		void InitCustomBindGroup();
        void ClearBindingProperty();
        void RefreshProperty(bool bCopyUniformBuffer = true);
        TArray<TSharedRef<FW::PropertyData>> PropertyDatasFromBinding();
        TArray<TSharedRef<FW::PropertyData>> PropertyDatasFromUniform(FW::UniformBuffer* InUb, bool Enabled);
        
    public:
        FW::AssetPtr<StShader> Shader;
		ShaderToyFormat Format;
		ShaderToyChannelDesc iChannelDesc0;
		ShaderToyChannelDesc iChannelDesc1;
		ShaderToyChannelDesc iChannelDesc2;
		ShaderToyChannelDesc iChannelDesc3;
        
	private:
		TSharedPtr<FW::PreviewViewPort> Preview = MakeShared<FW::PreviewViewPort>();
        //For Serialize
        TArray<uint8> CustomUniformBufferData;
        
        TUniquePtr<FW::UniformBuffer> CustomUniformBuffer;
        TRefCountPtr<FW::GpuBindGroupLayout> CustomBindLayout;
        TRefCountPtr<FW::GpuBindGroup> CustomBindGroup;
	};
}
