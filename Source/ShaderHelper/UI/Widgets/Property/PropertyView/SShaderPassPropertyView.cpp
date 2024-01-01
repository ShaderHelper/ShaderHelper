#include "CommonHeader.h"
#include "SShaderPassPropertyView.h"

namespace SH
{

	void SShaderPassPropertyView::Construct(const FArguments& InArgs)
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
			.OnContextMenuOpening(this, &SShaderPassPropertyView::CreateContextMenu)
			.IsExpandAll(true)
		];
	}

	TSharedRef<SWidget> SShaderPassPropertyView::GetCategoryMenu()
	{
		FMenuBuilder MenuBuilder{ true, TSharedPtr<FUICommandList>() };

		FSlateIcon Icon{ FAppStyle::Get().GetStyleSetName(), "Icons.Plus" };

		MenuBuilder.AddMenuEntry(
				FText::FromString("Float"),
				FText::GetEmpty(),
				Icon,
				FUIAction{ FExecuteAction::CreateSP(this, &SShaderPassPropertyView::AddUniform_Float) });

		MenuBuilder.AddMenuEntry(
			FText::FromString("Float2"),
			FText::GetEmpty(),
			Icon,
			FUIAction{ FExecuteAction::CreateSP(this, &SShaderPassPropertyView::AddUniform_Float2) });

		return MenuBuilder.MakeWidget();
	}

	void SShaderPassPropertyView::AddUniform_Float()
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

	void SShaderPassPropertyView::AddUniform_Float2()
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

	void SShaderPassPropertyView::ReCreateCustomUniformBuffer()
	{
		TArray<TSharedRef<PropertyData>> Uniforms;
		CustomUniformCategory->GetChildren(Uniforms);

		if (Uniforms.Num() == 0)
		{
			CustomUniformBuffer = nullptr;
		}
		else
		{
			UniformBufferBuilder Builder{ UniformBufferUsage::Persistant };
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

	void SShaderPassPropertyView::ReCreateCustomArgumentBuffer()
	{

		//if (!CustomUniformBuffer.IsValid()/*&& TODO*/)
		//{
		//	Renderer->UpdateCustomArgumentBuffer(nullptr);
		//	Renderer->UpdateCustomArgumentBufferLayout(nullptr);
		//}
		//else
		//{
		//	auto [NewArgumentBuffer, NewArgumentBufferLayout] = ArgumentBufferBuilder{ 1 }
		//		.AddUniformBuffer("Custom", CustomUniformBuffer, BindingShaderStage::Pixel)
		//		.Build();

		//	Renderer->UpdateCustomArgumentBuffer(AUX::TransOwnerShip(MoveTemp(NewArgumentBuffer)));
		//	Renderer->UpdateCustomArgumentBufferLayout(AUX::TransOwnerShip(MoveTemp(NewArgumentBufferLayout)));
		//}

		//if (SShaderEditorBox* ShaderEditorBox = ShaderEditor.Get())
		//{
		//	ShaderEditorBox->ReCompile();
		//}
	}

	void SShaderPassPropertyView::OnDeleteProperty(TSharedRef<PropertyData> InProperty)
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

	TSharedPtr<SWidget> SShaderPassPropertyView::CreateContextMenu()
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
			FUIAction{ FExecuteAction::CreateSP(this, &SShaderPassPropertyView::OnDeleteProperty, SelectedItems[0]) });

		return MenuBuilder.MakeWidget();
	}

}