#pragma once

namespace FW
{
	class FRAMEWORK_API SColorPicker : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SColorPicker) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
	};
}
