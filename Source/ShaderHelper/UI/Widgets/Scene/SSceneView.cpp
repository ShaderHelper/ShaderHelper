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
					auto Obj = CurRender->AddSceneObject<MeshSceneObject>();
					int32 Index = CurRender->SceneObjects.Num() - 1;
					if (auto* Mgr = GetUndoManager())
					{
						Mgr->PushCommand(MakeShared<AddSceneObjectCommand>(this, CurRender, Obj, Index));
					}
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
					auto Obj = CurRender->AddSceneObject<CameraSceneObject>();
					int32 Index = CurRender->SceneObjects.Num() - 1;
					if (auto* Mgr = GetUndoManager())
					{
						Mgr->PushCommand(MakeShared<AddSceneObjectCommand>(this, CurRender, Obj, Index));
					}
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
		if (bIgnoreSelectionChanged)
		{
			return;
		}

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		SceneObjectPtr NewSelectedObject = Item ? Item->Object : SceneObjectPtr{};

		if (NewSelectedObject.Get() == SelectedObject.Get())
		{
			// Re-show property for same object click
			if (SelectedObject)
			{
				ShEditor->ShowProperty(SelectedObject.Get());
			}
			return;
		}

		SceneObject* OldSelected = SelectedObject.Get();

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

		if (auto* Mgr = GetUndoManager())
		{
			Mgr->PushCommand(MakeShared<SelectionCommand>(this, OldSelected, SelectedObject.Get()));
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

		// Find index before removing
		int32 Index = INDEX_NONE;
		for (int32 i = 0; i < CurRender->SceneObjects.Num(); i++)
		{
			if (CurRender->SceneObjects[i].Get() == SelectedObject.Get())
			{
				Index = i;
				break;
			}
		}

		SceneObjectPtr RemovedObject = SelectedObject;
		CurRender->RemoveSceneObject(SelectedObject.Get());
		SelectedObject = nullptr;

		if (auto* Mgr = GetUndoManager())
		{
			Mgr->PushCommand(MakeShared<RemoveSceneObjectCommand>(this, CurRender, MoveTemp(RemovedObject), Index));
		}

		RefreshSceneItems();
		ShEditor->ForceRender();
	}

	void SSceneView::SelectObject(SceneObject* InObject)
	{
		if (InObject == SelectedObject.Get())
		{
			return;
		}

		SceneObject* OldSelected = SelectedObject.Get();
		SelectObjectInternal(InObject);

		if (auto* Mgr = GetUndoManager())
		{
			Mgr->PushCommand(MakeShared<SelectionCommand>(this, OldSelected, InObject));
		}
	}

	void SSceneView::SelectObjectInternal(SceneObject* InObject)
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
			bIgnoreSelectionChanged = true;
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
			bIgnoreSelectionChanged = false;
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
					int32 Index = CurRender->SceneObjects.Num() - 1;
					if (auto* Mgr = GetUndoManager())
					{
						Mgr->PushCommand(MakeShared<AddSceneObjectCommand>(this, CurRender, MeshObj, Index));
					}
				}
			}
			RefreshSceneItems();
			auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
			ShEditor->ForceRender();
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	SceneUndoManager* SSceneView::GetUndoManager()
	{
		if (!CurRender)
		{
			return nullptr;
		}

		auto& Mgr = UndoManagers.FindOrAdd(CurRender);
		if (!Mgr)
		{
			Mgr = MakeUnique<SceneUndoManager>();
		}
		return Mgr.Get();
	}

	void SSceneView::Undo()
	{
		if (auto* Mgr = GetUndoManager())
		{
			Mgr->UndoAction();
		}
	}

	void SSceneView::Redo()
	{
		if (auto* Mgr = GetUndoManager())
		{
			Mgr->RedoAction();
		}
	}

	bool SSceneView::CanUndo() const
	{
		if (!CurRender)
		{
			return false;
		}
		auto* MgrPtr = UndoManagers.Find(CurRender);
		return MgrPtr && (*MgrPtr)->CanUndo();
	}

	bool SSceneView::CanRedo() const
	{
		if (!CurRender)
		{
			return false;
		}
		auto* MgrPtr = UndoManagers.Find(CurRender);
		return MgrPtr && (*MgrPtr)->CanRedo();
	}

}
