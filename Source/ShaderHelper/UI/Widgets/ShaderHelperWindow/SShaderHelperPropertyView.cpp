#include "CommonHeader.h"
#include "SShaderHelperPropertyView.h"

namespace SH
{

	void SShaderHelperPropertyView::Construct(const FArguments& InArgs)
	{
		Renderer = InArgs._Renderer;
		ShaderEditor = InArgs._ShaderEditor;

		PropertyDatas = Renderer->GetBuiltInPropertyDatas();

		CustomPropertyCategory = MakeShared<PropertyCategory>("Custom");
		CustomPropertyCategory->SetAddMenuWidget(GetCategoryMenu());
		PropertyDatas.Add(CustomPropertyCategory.ToSharedRef());

		ChildSlot
		[
			SAssignNew(PropertyView, SPropertyView)
			.PropertyDatas(&PropertyDatas)
			.OnContextMenuOpening(this, &SShaderHelperPropertyView::CreateContextMenu)
			.IsExpandAll(true)
		];
	}

	TSharedRef<SWidget> SShaderHelperPropertyView::GetCategoryMenu()
	{
		FMenuBuilder MenuBuilder{ true, TSharedPtr<FUICommandList>() };

		FSlateIcon Icon{ FAppStyle::Get().GetStyleSetName(), "Icons.Plus" };

		MenuBuilder.AddMenuEntry(
				FText::FromString("Float"),
				FText::GetEmpty(),
				Icon,
				FUIAction{ FExecuteAction::CreateSP(this, &SShaderHelperPropertyView::AddUniform_Float) });

		MenuBuilder.AddMenuEntry(
			FText::FromString("Float2"),
			FText::GetEmpty(),
			Icon,
			FUIAction{ FExecuteAction::CreateSP(this, &SShaderHelperPropertyView::AddUniform_Float2) });

		return MenuBuilder.MakeWidget();
	}

	void SShaderHelperPropertyView::AddUniform_Float()
	{
		static int32 AddNum;

		if (!CustomUniformCategory)
		{
			CustomUniformCategory = MakeShared<PropertyCategory>("Uniform");
			PropertyView->ExpandItem(CustomUniformCategory.ToSharedRef());
			CustomPropertyCategory->AddChild(CustomUniformCategory.ToSharedRef());
		}

		FString NewUniformName = FString::Format(TEXT("Float_{0}"), { AddNum++ });
		auto NewUniformProperty = MakeShared<PropertyItem<float>>(NewUniformName, 0);
		NewUniformProperty->SetOnValueChanged([this, NewUniformProperty](float NewValue) {
			NewUniformProperty->UpdateUniformBuffer(CustomUniformBuffer.Get());
		});
		NewUniformProperty->SetOnDisplayNameChanged([this](const FString& NewDisplayName) {
			ReCreateCustomUniformBuffer();
		});

		CustomUniformCategory->AddChild(MoveTemp(NewUniformProperty));

		PropertyView->Refresh();
		ReCreateCustomUniformBuffer();
	}

	void SShaderHelperPropertyView::AddUniform_Float2()
	{
		static int32 AddNum;

		if (!CustomUniformCategory)
		{
			CustomUniformCategory = MakeShared<PropertyCategory>("Uniform");
			PropertyView->ExpandItem(CustomUniformCategory.ToSharedRef());
			CustomPropertyCategory->AddChild(CustomUniformCategory.ToSharedRef());
		}

		FString NewUniformName = FString::Format(TEXT("Float2_{0}"), { AddNum++ });
		auto NewUniformProperty = MakeShared<PropertyItem<Vector2f>>(NewUniformName, Vector2f{ 0 });
		NewUniformProperty->SetOnValueChanged([this, NewUniformProperty](Vector2f NewValue) {
			NewUniformProperty->UpdateUniformBuffer(CustomUniformBuffer.Get());
		});
		NewUniformProperty->SetOnDisplayNameChanged([this](const FString& NewDisplayName) {
			ReCreateCustomUniformBuffer();
		});

		CustomUniformCategory->AddChild(MoveTemp(NewUniformProperty));

		PropertyView->Refresh();
		ReCreateCustomUniformBuffer();
	}

	void SShaderHelperPropertyView::ReCreateCustomUniformBuffer()
	{
		TArray<TSharedRef<PropertyData>> Uniforms;
		CustomUniformCategory->GetChildren(Uniforms);

		if (Uniforms.Num() == 0)
		{
			CustomUniformBuffer = nullptr;
		}
		else
		{
			UniformBufferBuilder Builder{ "Custom", UniformBufferUsage::Persistant };
			for (const auto& Uniform : Uniforms)
			{
				Uniform->AddToUniformBuffer(Builder);
			}

			CustomUniformBuffer = AUX::TransOwnerShip(Builder.Build());
			for (const auto& Uniform : Uniforms)
			{
				Uniform->UpdateUniformBuffer(CustomUniformBuffer.Get());
			}
		}

		ReCreateCustomArgumentBuffer();
	}

	void SShaderHelperPropertyView::ReCreateCustomArgumentBuffer()
	{

		if (!CustomUniformBuffer.IsValid()/*&& TODO*/)
		{
			Renderer->UpdateCustomArgumentBuffer(nullptr);
			Renderer->UpdateCustomArgumentBufferLayout(nullptr);
		}
		else
		{
			auto [NewArgumentBuffer, NewArgumentBufferLayout] = ArgumentBufferBuilder{ 1 }
				.AddUniformBuffer(CustomUniformBuffer, BindingShaderStage::Pixel)
				.Build();

			Renderer->UpdateCustomArgumentBuffer(AUX::TransOwnerShip(MoveTemp(NewArgumentBuffer)));
			Renderer->UpdateCustomArgumentBufferLayout(AUX::TransOwnerShip(MoveTemp(NewArgumentBufferLayout)));
		}

		if (SShaderEditorBox* ShaderEditorBox = ShaderEditor.Get())
		{
			ShaderEditorBox->ReCompile();
		}
	}

	void SShaderHelperPropertyView::OnDeleteProperty(TSharedRef<PropertyData> InProperty)
	{
		int32 LastUniformCategoryChildrenNum = CustomUniformCategory->GetChildrenNum();
		InProperty->Remove();

		bool IsUniformProperty = LastUniformCategoryChildrenNum - CustomUniformCategory->GetChildrenNum() > 0;
		if (IsUniformProperty)
		{
			ReCreateCustomUniformBuffer();
			if (CustomUniformCategory->GetChildrenNum() == 0)
			{
				CustomUniformCategory->Remove();
				CustomUniformCategory = nullptr;
			}
		}

		PropertyView->Refresh();
	}

	TSharedPtr<SWidget> SShaderHelperPropertyView::CreateContextMenu()
	{
		TArray<TSharedRef<PropertyData>> SelectedItems = PropertyView->GetSelectedItems();
		if (SelectedItems.Num() == 0)
		{
			return nullptr;
		}

		FMenuBuilder MenuBuilder{ true, TSharedPtr<FUICommandList>() };

		MenuBuilder.AddMenuEntry(
			FText::FromString("Delete"),
			FText::GetEmpty(),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Delete"),
			FUIAction{ FExecuteAction::CreateSP(this, &SShaderHelperPropertyView::OnDeleteProperty, SelectedItems[0]) });

		return MenuBuilder.MakeWidget();
	}

}