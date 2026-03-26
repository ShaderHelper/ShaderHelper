#pragma once
#include "Editor/PreviewViewPort.h"
#include "AssetObject/Texture2D.h"
#include "AssetObject/Texture3D.h"
#include "AssetObject/TextureCube.h"
#include "AssetManager/AssetManager.h"

#include <Widgets/SViewport.h>

namespace FW
{
	class CubemapPreviewScene;
	class VolumePreviewScene;

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
		void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		AssetObject* GetTexture() const { return TextureAsset.Get(); }

		static void OpenTexturePreviewer(AssetPtr<AssetObject> InTexture, const FPointerEventHandler& InOnMouseButtonDown = {}, TSharedPtr<SWindow> InParentWindow = nullptr);

	private:
		void RefreshPreview();
		void UpdateMipOptions();
		GpuTexture* GetCurrentGpuTexture() const;

	private:
		AssetPtr<AssetObject> TextureAsset;
		TextureChannelFilter ChannelFilter = TextureChannelFilter::None;
		bool bView3D = false;
		int32 SelectedMipLevel = 0;
		int32 NumMipLevels = 1;
		TArray<TSharedPtr<int32>> MipOptions;
		TRefCountPtr<GpuTexture> PreviewTexture;
		TUniquePtr<CubemapPreviewScene> CubemapPreviewSceneInstance;
		TUniquePtr<VolumePreviewScene> VolumePreviewSceneInstance;
		TSharedPtr<PreviewViewPort> Preview;
		TSharedPtr<SHorizontalBox> Toolbar;
		TSharedPtr<SComboBox<TSharedPtr<int32>>> MipComboBox;
		FPointerEventHandler MouseButtonDownHandler;
		GpuTexture* CachedGpuTexture = nullptr;

		static inline TWeakPtr<STexturePreviewer> Instance;
	};
}
