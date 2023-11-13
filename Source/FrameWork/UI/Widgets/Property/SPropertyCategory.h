#pragma once

namespace FRAMEWORK
{
	class FRAMEWORK_API SPropertyCatergory : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPropertyCatergory) {}
			SLATE_ARGUMENT(FString, DisplayName)
			SLATE_ARGUMENT(TSharedPtr<SWidget>, AddMenuWidget)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedPtr<class ITableRow>& TableRow);
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	private:
		ITableRow* OwnerRowPtr;
	};
}


