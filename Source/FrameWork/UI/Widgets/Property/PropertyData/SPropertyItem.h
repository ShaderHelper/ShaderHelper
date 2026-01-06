#pragma once

namespace FW
{

	class FRAMEWORK_API SPropertyItem : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPropertyItem)
            : _DisplayName(nullptr)
            , _Indent(false)
        {}
            SLATE_ARGUMENT(TFunction<bool(const FText&)>, CanApplyName)
			SLATE_ARGUMENT(FText*, DisplayName)
			SLATE_ARGUMENT(TSharedPtr<SWidget>, ValueWidget)
            SLATE_ARGUMENT(bool, Indent)
			SLATE_ARGUMENT(TFunction<void(const FText&)>, OnDisplayNameChanged)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
    public:
        void AddWidget(TSharedPtr<SWidget> InWidget);
        
	private:
		FText* DisplayName;
        TSharedPtr<SHorizontalBox> HBox;
	};
}


