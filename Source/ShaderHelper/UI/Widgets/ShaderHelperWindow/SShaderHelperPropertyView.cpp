#include "CommonHeader.h"
#include "SShaderHelperPropertyView.h"

namespace SH
{

	void SShaderHelperPropertyView::Construct(const FArguments& InArgs)
	{
		Renderer = InArgs._Renderer;
		PropertyDatas = Renderer->GetBuiltInPropertyDatas();

		CustomPropertyCategory = MakeShared<PropertyCategory>("Custom");
		CustomPropertyCategory->SetAddMenuWidget(GetCategoryMenu());
		PropertyDatas.Add(CustomPropertyCategory.ToSharedRef());

		ChildSlot
		[
			SAssignNew(PropertyView, SPropertyView)
			.PropertyDatas(&PropertyDatas)
			//.OnContextMenuOpening()
			.IsExpandAll(true)
		];
	}

	TSharedRef<SWidget> SShaderHelperPropertyView::GetCategoryMenu()
	{
		FMenuBuilder MenuBuilder{ true, TSharedPtr<FUICommandList>() };

		MenuBuilder.AddMenuEntry(
				FText::FromString("Float"),
				FText::GetEmpty(),
				FSlateIcon(),
				FUIAction{ FExecuteAction::CreateSP(this, &SShaderHelperPropertyView::AddUniform_Float) });

		MenuBuilder.AddMenuEntry(
			FText::FromString("Float2"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction{ FExecuteAction::CreateSP(this, &SShaderHelperPropertyView::AddUniform_Float2) });

		return MenuBuilder.MakeWidget();
	}

	void SShaderHelperPropertyView::AddUniform_Float()
	{
		static int32 AddNum;

		if (!CustomUniformCategory)
		{
			CustomUniformCategory = MakeShared<PropertyCategory>("Uniform");
			CustomPropertyCategory->AddChild(CustomUniformCategory.ToSharedRef());
		}

		FString NewUniformName = FString::Format(TEXT("Float_{0}"), { AddNum });
		auto NewUniformProperty = MakeShared<PropertyNumber<float>>(NewUniformName, 0);
		NewUniformProperty->SetOnValueChanged([this, NewUniformName](float NewValue) {
			CustomUniformBuffer->GetMember<float>(NewUniformName) = NewValue;
		});

		CustomUniformCategory->AddChild(MoveTemp(NewUniformProperty));

		PropertyView->Refresh();
		ReCreateUniformBuffer();
	}

	void SShaderHelperPropertyView::AddUniform_Float2()
	{
		static int32 AddNum;

		if (!CustomUniformCategory)
		{
			CustomUniformCategory = MakeShared<PropertyCategory>("Uniform");
			CustomPropertyCategory->AddChild(CustomUniformCategory.ToSharedRef());
		}

		FString NewUniformName = FString::Format(TEXT("Float2_{0}"), { AddNum });
		auto NewUniformProperty = MakeShared<PropertyNumber<Vector2f>>(NewUniformName, Vector2f{ 0 });
		NewUniformProperty->SetOnValueChanged([this, NewUniformName](Vector2f NewValue) {
			CustomUniformBuffer->GetMember<Vector2f>(NewUniformName) = NewValue;
		});

		CustomUniformCategory->AddChild(MoveTemp(NewUniformProperty));

		PropertyView->Refresh();
		ReCreateUniformBuffer();
	}

	void SShaderHelperPropertyView::ReCreateUniformBuffer()
	{
		UniformBufferBuilder Builder{"Custom", UniformBufferUsage::Persistant };

		TArray<TSharedRef<PropertyData>> Uniforms;
		CustomUniformCategory->GetChildren(Uniforms);

		for (auto& Uniform : Uniforms)
		{
			Uniform->AddToUniformBuffer(Builder);
		}

		CustomUniformBuffer = AUX::TransOwnerShip(Builder.Build());

		ReCreateArgumentBuffer();
	}

	void SShaderHelperPropertyView::ReCreateArgumentBuffer()
	{
		//auto [NewArgumentBuffer, NewArgumentBufferLayout] = ArgumentBufferBuilder{ 1 }
		//	.AddUniformBuffer(CustomUniformBuffer, BindingShaderStage::Pixel)
		//	.Build();

		//Renderer->CustomArgumentBuffer = MoveTemp(NewArgumentBuffer);
		//Renderer->CustomArgumentBufferLayout = MoveTemp(NewArgumentBufferLayout);
	}

	void SShaderHelperPropertyView::OnDeleteProperty()
	{

	}

}