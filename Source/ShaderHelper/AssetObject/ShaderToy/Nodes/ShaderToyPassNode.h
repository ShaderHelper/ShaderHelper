#pragma once
#include "AssetObject/Graph.h"
#include "AssetObject/ShaderToy/Pins/ShaderToyPin.h"
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

	class ShaderToyPassNode : public FW::GraphNode, public DebuggableObject
	{
		REFLECTION_TYPE(ShaderToyPassNode)
	public:
		ShaderToyPassNode();
		~ShaderToyPassNode();

	public:
        void InitPins() override;
		void Serialize(FArchive& Ar) override;
        void PostLoad() override;
		TSharedPtr<SWidget> ExtraNodeWidget() override;
		FSlateColor GetNodeColor() const override { return FLinearColor{ 0.27f, 0.13f, 0.0f }; }
        FW::ExecRet Exec(FW::GraphExecContext& Context) override;

        
        void PostPropertyChanged(FW::PropertyData* InProperty) override;
        TArray<TSharedRef<FW::PropertyData>>* GetPropertyDatas() override;
    
		//ShDebuggableObject
		TRefCountPtr<FW::GpuTexture> OnStartDebugging() override;
		void OnFinalizePixel(const FW::Vector2u& PixelCoord) override;
		void OnEndDebuggging() override;
    private:
        void ClearBindingProperty();
        void RefreshProperty();
        TArray<TSharedRef<FW::PropertyData>> PropertyDatasFromBinding();
        TArray<TSharedRef<FW::PropertyData>> PropertyDatasFromUniform(FW::UniformBuffer* InUb, bool Enabled);
        
    public:
        FW::AssetPtr<StShader> Shader;
		ShaderToyFormat Format;
        
	private:
		TSharedPtr<FW::PreviewViewPort> Preview;
        //For Serialize
        uint32 CustomUbSize = 0;
        uint8* CustomUniformBufferData = nullptr;
        
        TUniquePtr<FW::UniformBuffer> CustomUniformBuffer;
        TRefCountPtr<FW::GpuBindGroupLayout> CustomBindLayout;
        TRefCountPtr<FW::GpuBindGroup> CustomBindGroup;
	};
}
