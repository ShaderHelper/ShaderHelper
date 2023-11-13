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
			SAssignNew(PropertyTree, SPropertyView)
			.PropertyDatas(&PropertyDatas)
			.IsExpandAll(true)
		];
	}

	TSharedRef<SWidget> SShaderHelperPropertyView::GetCategoryMenu() const
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

	void SShaderHelperPropertyView::AddUniform_Float() const
	{

	}

	void SShaderHelperPropertyView::AddUniform_Float2() const
	{

	}

}