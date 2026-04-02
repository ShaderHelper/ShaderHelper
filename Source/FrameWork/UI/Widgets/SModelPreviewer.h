#pragma once
#include "Editor/PreviewViewPort.h"
#include "AssetObject/Model.h"
#include "AssetManager/AssetManager.h"
#include "RenderResource/Camera.h"
#include "RenderResource/UniformBuffer.h"

#include <Widgets/SViewport.h>

namespace FW
{
	class FRAMEWORK_API SModelPreviewer : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SModelPreviewer) {}
			SLATE_ARGUMENT(AssetPtr<Model>, InModel)
			SLATE_EVENT(FPointerEventHandler, OnMouseButtonDown)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		Model* GetModel() const { return ModelAsset.Get(); }

		static void OpenModelPreviewer(AssetPtr<Model> InModel, const FPointerEventHandler& InOnMouseButtonDown = {}, TSharedPtr<SWindow> InParentWindow = nullptr);

	private:
		void InitRenderResources();
		void Render();
		void ResizeRenderTargetIfNeeded();

	private:
		AssetPtr<Model> ModelAsset;
		FPointerEventHandler MouseButtonDownHandler;

		TSharedPtr<PreviewViewPort> Preview;
		TRefCountPtr<GpuTexture> RenderTarget;
		TRefCountPtr<GpuTexture> MsaaRenderTarget;
		TRefCountPtr<GpuTexture> DepthTarget;

		Camera ViewCamera;
		Vector3f ModelCenter = { 0.0f, 0.0f, 0.0f };
		float ModelYaw = PI + PI / 8;
		float ModelPitch = 0.0f;
		bool bDragging = false;
		Vector2f LastMousePos = { 0.0f, 0.0f };
		FDelegateHandle ResizeHandlerHandle;

		TUniquePtr<UniformBuffer> ModelUniformBuffer;
		TRefCountPtr<GpuBindGroupLayout> BindGroupLayout;
		TRefCountPtr<GpuBindGroup> BindGroup;
		TRefCountPtr<GpuRenderPipelineState> Pipeline;

		static inline TWeakPtr<SModelPreviewer> Instance;
	};
}
