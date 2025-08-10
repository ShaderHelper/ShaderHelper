#pragma once

namespace FW
{
	class FRAMEWORK_API SPropertyCatergory : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPropertyCatergory)
		: _CategoryBrush(nullptr)
		{}
			SLATE_ARGUMENT(FString, DisplayName)
			SLATE_ARGUMENT(TSharedPtr<SWidget>, AddMenuWidget)
			SLATE_ARGUMENT(bool, IsRootCategory)
			SLATE_ARGUMENT( const FSlateBrush*, CategoryBrush )
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedPtr<class ITableRow>& TableRow);
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	private:
		ITableRow* OwnerRowPtr;
	};
}


