#pragma once
#include "AssetViewItem/AssetViewItem.h"

namespace FRAMEWORK
{

	class FRAMEWORK_API SAssetView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetView) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		void SetNewViewDirectory(const FString& NewViewDirectory);
		TSharedPtr<SWidget> CreateContextMenu();
		void ImportAsset();

	private:
		TArray<TSharedRef<AssetViewItem>> AssetViewItems;
		FString ImportAssetPath;
	};

}
