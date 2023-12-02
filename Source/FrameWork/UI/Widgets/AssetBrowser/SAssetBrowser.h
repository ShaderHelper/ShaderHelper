#pragma once

namespace FRAMEWORK
{
	class FRAMEWORK_API SAssetBrowser : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetBrowser) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
	};
}


