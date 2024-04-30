#pragma once

namespace FRAMEWORK
{
	class SDummyTab : public SDockTab
	{
	public:
		void Construct(const FArguments& InArgs)
		{
			SDockTab::Construct(InArgs);
			SBorder::ClearContent();

			SetRenderOpacity(0.0f);
			SetEnabled(false);
		}
	};
}