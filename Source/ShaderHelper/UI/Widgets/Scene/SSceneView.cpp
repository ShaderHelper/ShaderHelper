#include "CommonHeader.h"
#include "SSceneView.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "AssetObject/Render/CameraSceneObject.h"
#include "AssetObject/Model.h"
#include "AssetManager/AssetManager.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewItem.h"

#include <Widgets/Input/SComboButton.h>

using namespace FW;

namespace SH
{
	void SSceneView::Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SNew(SVerticalBox)
			.IsEnabled_Lambda([this]() {
				return CurRender;
			})
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SComboButton)
					.ButtonContent()
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
						.DesiredSizeOverride(FVector2D(12, 12))
					]
					.MenuContent()
					[
						MakeAddMenu()
					]
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				.Padding(4, 0, 4, 0)
				[
					SNew(SIconButton)
					.Icon(FAppStyle::Get().GetBrush("Icons.Delete"))
					.OnClicked_Lambda([this]() {
						DeleteSelected();
						return FReply::Handled();
					})
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				[
					SAssignNew(ListView, SListView<SceneObjectListItemPtr>)
					.ListItemsSource(&SceneItems)
					.SelectionMode(ESelectionMode::Single)
					.OnGenerateRow(this, &SSceneView::GenerateRowForItem)
					.OnContextMenuOpening(this, &SSceneView::CreateContextMenu)
					.OnMouseButtonClick_Lambda([this](SceneObjectListItemPtr Item) {
						if (Item && SelectedObject.Get() == Item->Object.Get())
						{
							OnSelectionChanged(Item, ESelectInfo::OnMouseClick);
						}
					})
					.OnSelectionChanged(this, &SSceneView::OnSelectionChanged)
				]
			]
		];
	}

	void SSceneView::SetRender(Render* InRender)
	{
		if (SelectedObject)
		{
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			if (ShEditor->GetCurPropertyObject() == SelectedObject.Get())
			{
				ShEditor->RefreshProperty(true);
			}
		}

		CurRender = InRender;
		SelectedObject = nullptr;
		RefreshSceneItems();
	}

	void SSceneView::RefreshSceneItems()
	{
		SceneItems.Reset();
		if (CurRender)
		{
			SceneItems.Reserve(CurRender->SceneObjects.Num());
			for (const SceneObjectPtr& SceneObject : CurRender->SceneObjects)
			{
				SceneItems.Add(MakeShared<SceneObjectListItem>(SceneObject));
			}
		}

		if (ListView)
		{
			ListView->RequestListRefresh();
		}
	}

	TSharedRef<SWidget> SSceneView::MakeAddMenu()
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		MenuBuilder.AddMenuEntry(
			LOCALIZATION("Mesh"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this]() {
				if (CurRender)
				{
					CurRender->AddSceneObject<MeshSceneObject>();
					RefreshSceneItems();
				}
			}))
		);

		MenuBuilder.AddMenuEntry(
			LOCALIZATION("Camera"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this]() {
				if (CurRender)
				{
					CurRender->AddSceneObject<CameraSceneObject>();
					RefreshSceneItems();
				}
			}))
		);

		return MenuBuilder.MakeWidget();
	}

	TSharedPtr<SWidget> SSceneView::CreateContextMenu()
	{
		if (!SelectedObject)
		{
			return SNullWidget::NullWidget;
		}

		FMenuBuilder MenuBuilder(true, nullptr);
		MenuBuilder.AddMenuEntry(
			LOCALIZATION("Delete"),
			FText::GetEmpty(),
			FSlateIcon(FAppStyle::Get().GetStyleSetName(), "GenericCommands.Delete"),
			FUIAction(FExecuteAction::CreateLambda([this]() {
				DeleteSelected();
			}))
		);

		return MenuBuilder.MakeWidget();
	}

	TSharedRef<ITableRow> SSceneView::GenerateRowForItem(SceneObjectListItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(STableRow<SceneObjectListItemPtr>, OwnerTable)
			.Padding(FMargin{4, 2})
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(0, 0, 4, 0)
				[
					SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.BulletPoint"))
					.DesiredSizeOverride(FVector2D(12, 12))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text_Lambda([Item]() {
						return Item && Item->Object ? Item->Object->ObjectName : FText::GetEmpty();
					})
				]
			];
	}

	void SSceneView::OnSelectionChanged(SceneObjectListItemPtr Item, ESelectInfo::Type SelectInfo)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SceneObjectPtr NewSelectedObject = Item ? Item->Object : SceneObjectPtr{};
		
		if (SelectedObject)
		{
			if (!ShEditor->IsPropertyLocked())
			{
				ShEditor->RefreshProperty(true);
			}
		}

		SelectedObject = MoveTemp(NewSelectedObject);

		if (SelectedObject)
		{
			ShEditor->ShowProperty(SelectedObject.Get());
		}
	}

	void SSceneView::DeleteSelected()
	{
		if (!CurRender || !SelectedObject)
		{
			return;
		}

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (ShEditor->GetCurPropertyObject() == SelectedObject.Get())
		{
			ShEditor->RefreshProperty(true);
		}

		CurRender->RemoveSceneObject(SelectedObject.Get());
		SelectedObject = nullptr;
		RefreshSceneItems();
		ShEditor->ForceRender();
	}

	void SSceneView::SelectObject(SceneObject* InObject)
	{
		if (InObject == SelectedObject.Get())
		{
			return;
		}

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());

		if (SelectedObject && !ShEditor->IsPropertyLocked())
		{
			ShEditor->RefreshProperty(true);
		}

		SelectedObject = InObject;

		// Update ListView selection
		if (ListView)
		{
			if (InObject)
			{
				for (const auto& Item : SceneItems)
				{
					if (Item->Object.Get() == InObject)
					{
						ListView->SetSelection(Item, ESelectInfo::Direct);
						break;
					}
				}
			}
			else
			{
				ListView->ClearSelection();
			}
		}

		if (SelectedObject)
		{
			ShEditor->ShowProperty(SelectedObject.Get());
		}
	}

	void SSceneView::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (CurRender && DragDropOp->IsOfType<AssetViewItemDragDropOp>())
		{
			TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
			for (const auto& DropFilePath : DropFilePaths)
			{
				MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
				if (DropAssetMetaType && DropAssetMetaType->IsType<Model>())
				{
					DragDropOp->SetCursorOverride(EMouseCursor::GrabHand);
					return;
				}
			}
			DragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
		}
	}

	void SSceneView::OnDragLeave(const FDragDropEvent& DragDropEvent)
	{
		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		DragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
	}

	FReply SSceneView::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		if (!CurRender)
		{
			return FReply::Unhandled();
		}

		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (DragDropOp->IsOfType<AssetViewItemDragDropOp>())
		{
			TArray<FString> DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
			for (const auto& DropFilePath : DropFilePaths)
			{
				MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
				if (DropAssetMetaType && DropAssetMetaType->IsType<Model>())
				{
					AssetPtr<Model> ModelAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<Model>(DropFilePath);
					auto MeshObj = CurRender->AddSceneObject<MeshSceneObject>();
					MeshObj->ModelAsset = MoveTemp(ModelAsset);
				}
			}
			RefreshSceneItems();
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			ShEditor->ForceRender();
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}
}
