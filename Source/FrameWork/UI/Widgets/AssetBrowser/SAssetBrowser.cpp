#include "CommonHeader.h"
#include "SAssetBrowser.h"
#include "SAssetView.h"
#include "SDirectoryTree.h"

namespace FRAMEWORK
{

	void SAssetBrowser::Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SSplitter)
			.PhysicalSplitterHandleSize(2.0f)
			+ SSplitter::Slot()
			.Value(0.25f)
			[
				SNew(SBorder)
				[
					SNew(SDirectoryTree)
					.DirectoryShowed(InArgs._DirectoryShowed)
				]
				
			]
			+ SSplitter::Slot()
			.Value(0.75f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				[
					SNew(SAssetView)
				]
			]
		];
	}

}
