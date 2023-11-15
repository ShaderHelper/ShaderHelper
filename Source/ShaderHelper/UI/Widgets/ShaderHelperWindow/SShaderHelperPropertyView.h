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
		TSharedRef<SWidget> GetCategoryMenu();
		void AddUniform_Float();
		void AddUniform_Float2();
		void ReCreateUniformBuffer();
		void ReCreateArgumentBuffer();
		void OnDeleteProperty();

	private:
		TSharedPtr<SPropertyView> PropertyView;
		TArray<TSharedRef<PropertyData>> PropertyDatas;
		
		TSharedPtr<UniformBuffer> CustomUniformBuffer;
		TSharedPtr<PropertyCategory> CustomPropertyCategory;
		TSharedPtr<PropertyCategory> CustomUniformCategory;
		ShRenderer* Renderer = nullptr;
	};
}