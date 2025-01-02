#pragma once

namespace FW
{

	class FRAMEWORK_API SPropertyItem : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPropertyItem) {}
			SLATE_ARGUMENT(FString, DisplayName)
			SLATE_ARGUMENT(TSharedPtr<SWidget>, ValueWidget)
			SLATE_ARGUMENT(bool, IsEnabled)
			SLATE_ARGUMENT(TFunction<void(const FString&)>, OnDisplayNameChanged)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

		FText GetNameText() const { return DisplayNameText; }
	private:
		FText DisplayNameText;
		TFunction<void(const FString&)> OnDisplayNameChanged;
	};
}


