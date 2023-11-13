#pragma once
#include "UI/Widgets/Property/SPropertyView.h"
#include "Renderer/ShRenderer.h"

namespace SH
{
	class SShaderHelperPropertyView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SShaderHelperPropertyView)
			: _Renderer(nullptr)
		{}
			SLATE_ARGUMENT(ShRenderer*, Renderer)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		TSharedRef<SWidget> GetCategoryMenu() const;
		void AddUniform_Float() const;
		void AddUniform_Float2() const;

	private:
		TSharedPtr<SPropertyView> PropertyTree;
		TArray<TSharedRef<PropertyData>> PropertyDatas;
		
		TSharedPtr<PropertyCategory> CustomPropertyCategory;
		ShRenderer* Renderer = nullptr;
	};
}