#pragma once
#include "AssetViewItem/AssetViewItem.h"

namespace FRAMEWORK
{

	class FRAMEWORK_API SAssetView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetView) {}
			SLATE_EVENT(FOnContextMenuOpening, OnContextMenuOpening)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	private:
		TArray<TSharedRef<AssetViewItem>> AssetViewItems;
	};

}
