#pragma once
#include "UI/Widgets/Property/SPropertyView.h"
#include "Renderer/ShRenderer.h"
#include "UI/Widgets/ShaderCodeEditor/SShaderEditorBox.h"

namespace SH
{
	class SShaderHelperPropertyView : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SShaderHelperPropertyView)
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
		void OnDeleteProperty(TSharedRef<PropertyData> InProperty);
		TSharedPtr<SWidget> CreateContextMenu();

	private:
		TAttribute<SShaderEditorBox*> ShaderEditor;

		TSharedPtr<SPropertyView> PropertyView;
		TArray<TSharedRef<PropertyData>> PropertyDatas;
		
		TSharedPtr<UniformBuffer> CustomUniformBuffer;
		TSharedPtr<PropertyCategory> CustomPropertyCategory;
		TSharedPtr<PropertyCategory> CustomUniformCategory;
		ShRenderer* Renderer = nullptr;
	};
}