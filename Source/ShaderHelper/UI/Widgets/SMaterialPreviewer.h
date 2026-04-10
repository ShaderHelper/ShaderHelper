#pragma once
#include "Editor/PreviewViewPort.h"
#include "AssetObject/Material.h"
#include "Renderer/MaterialPreviewRenderer.h"

#include <Widgets/SViewport.h>

namespace SH
{
	class SMaterialPreviewer : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SMaterialPreviewer) {}
			SLATE_ARGUMENT(FW::AssetPtr<Material>, InMaterial)
			SLATE_EVENT(FPointerEventHandler, OnMouseButtonDown)
		SLATE_END_ARGS()

		~SMaterialPreviewer();
		void Construct(const FArguments& InArgs);

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

		static void OpenMaterialPreviewer(FW::AssetPtr<Material> InMaterial, const FPointerEventHandler& InOnMouseButtonDown = {}, TSharedPtr<SWindow> InParentWindow = nullptr);

	private:
		void Render();

	private:
		FW::AssetPtr<Material> MaterialAsset;
		FPointerEventHandler MouseButtonDownHandler;
		TSharedPtr<FW::PreviewViewPort> Preview;
		TArray<TSharedPtr<MaterialPreviewPrimitive>> PreviewPrimitiveOptions;
		TUniquePtr<MaterialPreviewRenderer> Renderer;
		TSharedPtr<STextBlock> ErrorText;
		FDelegateHandle ResizeHandlerHandle;
		FDelegateHandle MaterialChangedHandle;
		bool bDragging = false;
		FW::Vector2f LastMousePos = { 0.0f, 0.0f };
		float OrbitYaw = 0.0f;
		float OrbitPitch = 0.0f;
		float CameraDistance = 2.5f;
		MaterialPreviewPrimitive PreviewPrimitive = MaterialPreviewPrimitive::Sphere;

		static inline TWeakPtr<SMaterialPreviewer> Instance;
	};
}
