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
		void OnDeleteProperty(TSharedRef<FW::PropertyData> InProperty);
		TSharedPtr<SWidget> CreateContextMenu();

	private:
		TAttribute<SShaderEditorBox*> ShaderEditor;

		TSharedPtr<FW::SPropertyView> PropertyView;
		TArray<TSharedRef<FW::PropertyData>> PropertyDatas;
		
		TSharedPtr<FW::UniformBuffer> CustomUniformBuffer;
		TSharedPtr<FW::PropertyCategory> CustomPropertyCategory;
		TSharedPtr<FW::PropertyCategory> CustomUniformCategory;
		ShRenderer* Renderer = nullptr;
	};
}
