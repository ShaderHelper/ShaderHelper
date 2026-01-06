#pragma once

namespace FW
{
	enum class EColorDisplayMode : int32
	{
		RGB = 0,
		HSV = 1
	};

	class FRAMEWORK_API SColorPicker : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SColorPicker)
			: _TargetColorAttribute()
			, _OnColorChanged()
			, _OnDestroyed()
			, _ShowAlpha(true), _PreviewSrgb(false)
		{}
			SLATE_ATTRIBUTE(FLinearColor, TargetColorAttribute)
			SLATE_EVENT(FOnLinearColorValueChanged, OnColorChanged)
			SLATE_EVENT(FSimpleDelegate, OnDestroyed)
			SLATE_ARGUMENT(bool, ShowAlpha)
			SLATE_ARGUMENT(bool, PreviewSrgb)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		~SColorPicker();

	private:
		FLinearColor CurrentColor;
		FOnLinearColorValueChanged OnColorChanged;
		FSimpleDelegate OnDestroyed;
		bool bShowAlpha = true;
		
		void HandleWheelColorChanged(FLinearColor NewColor);
		void HandleAlphaChanged(float NewAlpha);
		void HandleRGBChanged(float NewValue, int32 Component);
		void HandleHSVChanged(float NewValue, int32 Component);
		
		EColorDisplayMode DisplayMode = EColorDisplayMode::RGB;
		bool bPreviewSrgb;
	};
}
