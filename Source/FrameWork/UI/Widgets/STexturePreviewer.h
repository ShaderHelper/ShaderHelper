#pragma once
#include "Editor/PreviewViewPort.h"
#include "AssetObject/Texture2D.h"
#include "AssetObject/TextureCube.h"
#include "AssetManager/AssetManager.h"

#include <Widgets/SViewport.h>

namespace FW
{
	class CubemapPreviewScene;

	enum class TextureChannelFilter
	{
		None,
		R,
		G,
		B,
		A
	};

	class FRAMEWORK_API STexturePreviewer : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(STexturePreviewer) {}
			SLATE_ARGUMENT(AssetPtr<AssetObject>, InTexture)
			SLATE_EVENT(FPointerEventHandler, OnMouseButtonDown)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		AssetObject* GetTexture() const { return TextureAsset.Get(); }

		static void OpenTexturePreviewer(AssetPtr<AssetObject> InTexture, const FPointerEventHandler& InOnMouseButtonDown = {}, TSharedPtr<SWindow> InParentWindow = nullptr);

	private:
		void RefreshPreview();

	private:
		AssetPtr<AssetObject> TextureAsset;
		TextureChannelFilter ChannelFilter = TextureChannelFilter::None;
		bool bView3D = false;
		TRefCountPtr<GpuTexture> PreviewTexture;
		TUniquePtr<CubemapPreviewScene> CubemapPreviewSceneInstance;
		TSharedPtr<PreviewViewPort> Preview;
		TSharedPtr<SHorizontalBox> Toolbar;
		FPointerEventHandler MouseButtonDownHandler;

		static inline TWeakPtr<STexturePreviewer> Instance;
	};
}
