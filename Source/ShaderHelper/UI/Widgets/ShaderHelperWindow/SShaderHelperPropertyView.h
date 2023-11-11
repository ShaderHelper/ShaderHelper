#pragma once
#include "UI/Widgets/Property/SPropertyView.h"
#include "Renderer/ShRenderer.h"

namespace SH
{
	class SShaderHelperPropertyView : public SCompoundWidget
	{
		using PropertyDataType = TSharedRef<PropertyData>;
	public:
		SLATE_BEGIN_ARGS(SShaderHelperPropertyView)
			: _Renderer(nullptr)
		{}
			SLATE_ARGUMENT(const ShRenderer*, Renderer)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

	private:
		TSharedPtr<SPropertyView<PropertyDataType>> PropertyTree;
		TArray<PropertyDataType> PropertyDatas;
	};
}