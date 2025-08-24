#pragma once

namespace FW
{
	class SShWindow : public SWindow
	{
	public:
		void Construct(const FArguments& InArgs)
		{
			SWindow::Construct(InArgs);
		}

		void SetKeyDownHandler(const FOnKeyDown& InHandler)
		{
			KeyDownHandler = InHandler;
		}

		FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override
		{
			if(KeyDownHandler.IsBound())
			{
				return KeyDownHandler.Execute(MyGeometry, InKeyEvent);
			}
			return FReply::Unhandled();
		}

	private:
		FOnKeyDown KeyDownHandler;
	};
}
