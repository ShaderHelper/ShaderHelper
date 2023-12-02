#pragma once

namespace FRAMEWORK
{
	class FRAMEWORK_API SAssetView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SAssetView) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
	};
}
