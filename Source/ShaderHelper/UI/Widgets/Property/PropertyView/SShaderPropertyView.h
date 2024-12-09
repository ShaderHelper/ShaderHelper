#pragma once
#include "UI/Widgets/Property/PropertyView/SPropertyView.h"
#include "Renderer/ShRenderer.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"

namespace SH
{
	class SShaderPropertyView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SShaderPropertyView)
			: _Renderer(nullptr)
		{}
			SLATE_ARGUMENT(ShRenderer*, Renderer)
			SLATE_ATTRIBUTE(SShaderEditorBox*, ShaderEditor)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		TSharedRef<SWidget> GetCategoryMenu();
		void AddUniform_Float();
		void AddUniform_Float2();
		void ReCreateCustomUniformBuffer();
		void ReCreateCustomArgumentBuffer();
		void OnDeleteProperty(TSharedRef<FRAMEWORK::PropertyData> InProperty);
		TSharedPtr<SWidget> CreateContextMenu();

	private:
		TAttribute<SShaderEditorBox*> ShaderEditor;

		TSharedPtr<FRAMEWORK::SPropertyView> PropertyView;
		TArray<TSharedRef<FRAMEWORK::PropertyData>> PropertyDatas;
		
		TSharedPtr<FRAMEWORK::UniformBuffer> CustomUniformBuffer;
		TSharedPtr<FRAMEWORK::PropertyCategory> CustomPropertyCategory;
		TSharedPtr<FRAMEWORK::PropertyCategory> CustomUniformCategory;
		ShRenderer* Renderer = nullptr;
	};
}
