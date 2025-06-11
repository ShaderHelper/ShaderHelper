#pragma once
#include <Framework/Text/BaseTextLayoutMarshaller.h>
#include <Widgets/Input/SMultiLineEditableTextBox.h>
#include <Widgets/Input/SSearchBox.h>

namespace FW
{

	class FLogWidgetMarshaller : public FBaseTextLayoutMarshaller
	{
	public:
		FLogWidgetMarshaller();
		~FLogWidgetMarshaller();

		virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout, TArray<FTextLayout::FLineModel>&& OldLineModels) override;
		virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;
	private:
		FTextLayout* TextLayout;
	};

	class FRAMEWORK_API SLogWidget : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SLogWidget) {}
			SLATE_ARGUMENT(TSharedPtr<ITextLayoutMarshaller>, Marshaller)
			SLATE_NAMED_SLOT(FArguments, ExtraContentArea)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
	protected:
		TSharedPtr<SMultiLineEditableTextBox> MessagesTextBox;
		TSharedPtr<ITextLayoutMarshaller> Marshaller;
	};
}


