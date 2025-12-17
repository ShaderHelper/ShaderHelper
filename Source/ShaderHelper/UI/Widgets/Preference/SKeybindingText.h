#pragma once
#include <Widgets/Text/SInlineEditableTextBlock.h>

namespace SH
{
	class SKeybindingText : public SInlineEditableTextBlock
	{
		SLATE_BEGIN_ARGS(SKeybindingText) {}
			SLATE_ARGUMENT(TSharedPtr<FUICommandInfo>, Command)
		SLATE_END_ARGS()
	public:
		void Construct(const FArguments& InArgs);

	private:
		TSharedPtr<FUICommandInfo> Command;
		FInputChord EditingInputChord;
	};
}
