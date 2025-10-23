#include "CommonHeader.h"
#include "SColorPicker.h"
#include <Widgets/Colors/SColorWheel.h>

namespace FW
{
	void SColorPicker::Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SColorWheel)
		];
	}
}