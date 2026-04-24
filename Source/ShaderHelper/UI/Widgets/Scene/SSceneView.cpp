#include "CommonHeader.h"
#include "SSceneView.h"
#include "AssetObject/Render/MeshSceneObject.h"
#include "AssetObject/Render/CameraSceneObject.h"
#include "AssetObject/Model.h"
#include "AssetManager/AssetManager.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"
#include "Editor/SceneViewCommands.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "UI/Widgets/Misc/CommonTableRow.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewItem.h"

#include <Widgets/Input/SComboButton.h>
#include <Widgets/Text/SInlineEditableTextBlock.h>

using namespace FW;

namespace SH
{
	static const FSlateBrush* GetSceneObjectIcon(const SceneObject* Obj)
	{
		if (dynamic_cast<const CameraSceneObject*>(Obj))
			return FShaderHelperStyle::Get().GetBrush("Icons.SceneObject.Camera");
		if (dynamic_cast<const MeshSceneObject*>(Obj))
			return FShaderHelperStyle::Get().GetBrush("Icons.SceneObject.Mesh");
		return nullptr;
	}

	static bool HasDroppableModelAsset(const TSharedPtr<FDragDropOperation>& DragDropOp)
	{
		if (!DragDropOp || !DragDropOp->IsOfType<AssetViewItemDragDropOp>())
		{
			return false;
		}

		const TArray<FString>& DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
		for (const FString& DropFilePath : DropFilePaths)
		{
			MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
			if (DropAssetMetaType && DropAssetMetaType->IsType<Model>())
			{
				return true;
			}
		}

		return false;
	}

	static bool HandleModelAssetDrop(SSceneView* SceneView, const TSharedPtr<FDragDropOperation>& DragDropOp, SceneObject* Parent = nullptr)
	{
		Render* Render = SceneView ? SceneView->GetRender() : nullptr;
		if (!Render || !HasDroppableModelAsset(DragDropOp))
		{
			return false;
		}

		bool bAddedObject = false;
		const TArray<FString>& DropFilePaths = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Paths;
		for (const FString& DropFilePath : DropFilePaths)
		{
			MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
			if (!DropAssetMetaType || !DropAssetMetaType->IsType<Model>())
			{
				continue;
			}

			AssetPtr<Model> ModelAsset = TSingleton<AssetManager>::Get().LoadAssetByPath<Model>(DropFilePath);
			auto MeshObj = Render->AddSceneObject<MeshSceneObject>(Parent);
			MeshObj->ObjectName = ModelAsset->ObjectName;
			MeshObj->ModelAsset = MoveTemp(ModelAsset);
			int32 Index = Render->SceneObjects.Num() - 1;
			int32 ParentChildIndex = Parent ? Parent->Children.Num() - 1 : INDEX_NONE;
			SceneView->GetUndoManager().PushCommand(MakeShared<AddSceneObjectCommand>(SceneView, Render, MeshObj, Index, Parent, ParentChildIndex));
			bAddedObject = true;
		}

		if (!bAddedObject)
		{
			return false;
		}

		SceneView->RefreshSceneItems();
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();
		return true;
	}

	class SSceneObjectRow : public FW::SDropTargetTableRow<SceneObjectTreeItemPtr>
	{
	public:
		SLATE_BEGIN_ARGS(SSceneObjectRow) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, SceneObjectTreeItemPtr InItem, SSceneView* InSceneView,
			TSharedRef<STreeView<SceneObjectTreeItemPtr>> InTreeView,
			const TSharedRef<STableViewBase>& InOwnerTableView)
		{
			Item = InItem;
			SceneViewPtr = InSceneView;
			TreeViewWeak = InTreeView;

			FW::SDropTargetTableRow<SceneObjectTreeItemPtr>::Construct(
				STableRow<SceneObjectTreeItemPtr>::FArguments()
				.Padding(FMargin{2, 1})
				.OnDrop(FOnTableRowDrop::CreateLambda([this](const FDragDropEvent& DragDropEvent) -> FReply {
					if (!SceneViewPtr || !Item || !Item->Object) return FReply::Unhandled();

					if (HasDroppableModelAsset(DragDropEvent.GetOperation()))
					{
						return HandleModelAssetDrop(SceneViewPtr, DragDropEvent.GetOperation(), Item->Object.Get()) ? FReply::Handled() : FReply::Unhandled();
					}

					TArray<SceneObjectTreeItemPtr> DraggedItems;
					if (auto TreeViewPin = TreeViewWeak.Pin())
					{
						TreeViewPin->GetSelectedItems(DraggedItems);
					}
					if (DraggedItems.IsEmpty() && Item)
					{
						DraggedItems.Add(Item);
					}

					SceneViewPtr->ReparentSceneObjects(DraggedItems, Item->Object.Get());
					return FReply::Handled();
				}))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0, 0, 4, 0)
					[
						SNew(SImage)
						.ColorAndOpacity(FStyleColors::Foreground)
						.Image_Lambda([InItem]() -> const FSlateBrush* {
							return GetSceneObjectIcon(InItem ? InItem->Object.Get() : nullptr);
						})
						.DesiredSizeOverride(FVector2D(14, 14))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SAssignNew(InItem->InlineTextBlock, SInlineEditableTextBlock)
						.Text_Lambda([InItem]() {
							return InItem && InItem->Object ? InItem->Object->ObjectName : FText::GetEmpty();
						})
						.IsSelected_Lambda([InSceneView, InItem]() -> bool {
							return InItem && InSceneView->IsObjectSelected(InItem->Object.Get());
						})
						.OnTextCommitted_Lambda([InSceneView, InItem](const FText& NewText, ETextCommit::Type CommitType) {
							if (CommitType == ETextCommit::OnCleared || !InItem || !InItem->Object || NewText.IsEmpty())
							{
								return;
							}
							FText OldName = InItem->Object->ObjectName;
							if (!OldName.EqualTo(NewText))
							{
								InItem->Object->ObjectName = NewText;
								InItem->Object->GetOuterMost()->MarkDirty();
								if (InSceneView->GetRender())
								{
									InSceneView->GetUndoManager().PushCommand(MakeShared<RenameSceneObjectCommand>(InSceneView, InItem->Object, OldName, NewText));
								}
							}
						})
					]
				],
				InOwnerTableView
			);

			SetDragFilter([this](const TSharedPtr<FDragDropOperation>& Op) -> bool {
				if (!Op) return false;
				if (HasDroppableModelAsset(Op)) return Item && Item->Object;
				if (!Op->IsOfType<ShObjectDragDropOp>()) return false;
				const auto ShOp = StaticCastSharedPtr<ShObjectDragDropOp>(Op);
				if (!ShOp->Object || !dynamic_cast<SceneObject*>(ShOp->Object)) return false;
				return IsValidDropTarget();
			});
		}

		FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			FReply Reply = SDropTargetTableRow::OnMouseButtonDown(MyGeometry, MouseEvent);

			if (SceneViewPtr && Item && Item->Object
				&& MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
				if (SceneViewPtr->IsObjectSelected(Item->Object.Get()))
				{
					ShEditor->ShowProperty(Item->Object.Get());
				}
			}
			return Reply;
		}

		FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (!Item || !Item->Object) return FReply::Unhandled();
			return FReply::Handled().BeginDragDrop(ShObjectDragDropOp::New(Item->Object.Get()));
		}

	private:
		bool IsValidDropTarget() const
		{
			if (!Item || !Item->Object) return false;
			SceneObject* Target = Item->Object.Get();

			TArray<SceneObjectTreeItemPtr> SelectedItems;
			if (auto TreeViewPin = TreeViewWeak.Pin())
			{
				TreeViewPin->GetSelectedItems(SelectedItems);
			}
			if (SelectedItems.IsEmpty()) return false;

			for (const SceneObjectTreeItemPtr& SelItem : SelectedItems)
			{
				if (!SelItem || !SelItem->Object) continue;
				SceneObject* Dragged = SelItem->Object.Get();

				if (Dragged == Target) return false;

				SceneObject* Cursor = Target->GetParent();
				while (Cursor)
				{
					if (Cursor == Dragged) return false;
					Cursor = Cursor->GetParent();
				}
			}
			return true;
		}

		SceneObjectTreeItemPtr Item;
		SSceneView* SceneViewPtr = nullptr;
		TWeakPtr<STreeView<SceneObjectTreeItemPtr>> TreeViewWeak;
	};

	template<typename T>
	void SSceneView::DoAddSceneObject(SceneObject* InParent)
	{
		if (!CurRender)
		{
			return;
		}

		auto Obj = CurRender->AddSceneObject<T>(InParent);
		int32 Index = CurRender->SceneObjects.Num() - 1;
		int32 ParentChildIndex = InParent ? InParent->Children.Num() - 1 : INDEX_NONE;

		GetUndoManager().PushCommand(MakeShared<AddSceneObjectCommand>(this, CurRender, Obj, Index, InParent, ParentChildIndex));
		RefreshSceneItems();
		SelectObjectInternal(Obj.Get());
		FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float) -> bool {
			BeginRenameSelected();
			return false;
		}));
	}

	void SSceneView::Construct(const FArguments& InArgs)
	{
		CommandList = MakeShared<FUICommandList>();
		CommandList->MapAction(
			SceneViewCommands::Get().DeleteObject,
			FExecuteAction::CreateLambda([this]() { DeleteSelected(); }),
			FCanExecuteAction::CreateLambda([this]() { return !GetSelectedObjects().IsEmpty(); })
		);
		CommandList->MapAction(
			SceneViewCommands::Get().RenameObject,
			FExecuteAction::CreateLambda([this]() { BeginRenameSelected(); }),
			FCanExecuteAction::CreateLambda([this]() { return !GetSelectedObjects().IsEmpty(); })
		);
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
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					SNew(SComboButton)
					.ButtonContent()
					[
						SNew(SImage)
						.ColorAndOpacity(FStyleColors::Foreground)
						.Image(FAppStyle::Get().GetBrush("Icons.Plus"))
						.DesiredSizeOverride(FVector2D(12, 12))
					]
					.MenuContent()
					[
						MakeAddMenu()
					]
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
				[
					SAssignNew(TreeView, STreeView<SceneObjectTreeItemPtr>)
					.TreeItemsSource(&RootItems)
					.SelectionMode(ESelectionMode::Multi)
					.OnGetChildren(this, &SSceneView::GetChildrenForItem)
					.OnGenerateRow(this, &SSceneView::GenerateRowForItem)
					.OnContextMenuOpening(this, &SSceneView::CreateContextMenu)
					.OnSelectionChanged(this, &SSceneView::OnSelectionChanged)
				]
			]
		];
	}

	void SSceneView::SetRender(Render* InRender)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (SceneObject* PropertyObject = dynamic_cast<SceneObject*>(ShEditor->GetCurPropertyObject()))
		{
			if (IsObjectSelected(PropertyObject))
			{
				ShEditor->RefreshProperty(true);
			}
		}

		if (CurRender != InRender)
		{
			UndoManager.Clear();
		}

		CurRender = InRender;
		LastSelectedObjects.Empty();
		bIgnoreSelectionChanged = true;
		TreeView->ClearSelection();
		bIgnoreSelectionChanged = false;
		RefreshSceneItems();
	}

	void SSceneView::RefreshSceneItems()
	{
		TArray<SceneObject*> PreviouslySelectedObjects = GetSelectedObjects();
		SceneObject* ScrollObject = PreviouslySelectedObjects.IsEmpty() ? nullptr : PreviouslySelectedObjects[0];

		// Snapshot expansion state before rebuilding (new items lose identity tracked by STreeView)
		TArray<SceneObject*> PreviouslyExpandedObjects;
		for (auto& Pair : AllItems)
		{
			if (TreeView->IsItemExpanded(Pair.Value))
			{
				PreviouslyExpandedObjects.Add(Pair.Key);
			}
		}

		RootItems.Reset();
		AllItems.Reset();

		if (CurRender)
		{
			for (const SceneObjectPtr& Obj : CurRender->SceneObjects)
			{
				auto Item = MakeShared<SceneObjectTreeItem>(Obj);
				AllItems.Add(Obj.Get(), Item);
			}

			for (const SceneObjectPtr& Obj : CurRender->SceneObjects)
			{
				SceneObjectTreeItemPtr Item = AllItems[Obj.Get()];
				if (!Obj->Parent.IsValid())
				{
					RootItems.Add(Item);
				}
				else if (SceneObjectTreeItemPtr* ParentItemPtr = AllItems.Find(Obj->Parent.Get()))
				{
					(*ParentItemPtr)->Children.Add(Item);
				}
				else
				{
					RootItems.Add(Item);
				}
			}
		}

		TreeView->RequestTreeRefresh();

		// Restore expansion state
		for (SceneObject* Obj : PreviouslyExpandedObjects)
		{
			if (SceneObjectTreeItemPtr* ItemPtr = AllItems.Find(Obj))
			{
				TreeView->SetItemExpansion(*ItemPtr, true);
			}
		}

		bIgnoreSelectionChanged = true;
		TreeView->ClearSelection();

		for (SceneObject* Obj : PreviouslySelectedObjects)
		{
			if (SceneObjectTreeItemPtr* ItemPtr = AllItems.Find(Obj))
			{
				TreeView->SetItemSelection(*ItemPtr, true, ESelectInfo::Direct);
			}
		}

		if (!ScrollObject && !PreviouslySelectedObjects.IsEmpty())
		{
			ScrollObject = PreviouslySelectedObjects[0];
		}
		if (ScrollObject)
		{
			if (SceneObjectTreeItemPtr* ItemPtr = AllItems.Find(ScrollObject))
			{
				TreeView->RequestScrollIntoView(*ItemPtr);
			}
		}
		bIgnoreSelectionChanged = false;
	}

	TSharedRef<SWidget> SSceneView::MakeAddMenu()
	{
		FMenuBuilder MenuBuilder(true, nullptr);

		MenuBuilder.AddMenuEntry(
			LOCALIZATION("Empty"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this]() {
				DoAddSceneObject<SceneObject>();
			}))
		);

		MenuBuilder.AddMenuEntry(
			LOCALIZATION("Mesh"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this]() {
				DoAddSceneObject<MeshSceneObject>();
			}))
		);

		MenuBuilder.AddMenuEntry(
			LOCALIZATION("Camera"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this]() {
				DoAddSceneObject<CameraSceneObject>();
			}))
		);

		return MenuBuilder.MakeWidget();
	}

	TSharedRef<SWidget> SSceneView::MakeAddChildMenu()
	{
		FMenuBuilder MenuBuilder(true, nullptr);
		TArray<SceneObject*> SelectedObjects = GetSelectedObjects();
		SceneObject* Parent = SelectedObjects.IsEmpty() ? nullptr : SelectedObjects[0];

		MenuBuilder.AddMenuEntry(
			LOCALIZATION("Empty"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, Parent]() {
				DoAddSceneObject<SceneObject>(Parent);
			}))
		);

		MenuBuilder.AddMenuEntry(
			LOCALIZATION("Mesh"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, Parent]() {
				DoAddSceneObject<MeshSceneObject>(Parent);
			}))
		);

		MenuBuilder.AddMenuEntry(
			LOCALIZATION("Camera"),
			FText::GetEmpty(),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([this, Parent]() {
				DoAddSceneObject<CameraSceneObject>(Parent);
			}))
		);

		return MenuBuilder.MakeWidget();
	}

	TSharedPtr<SWidget> SSceneView::CreateContextMenu()
	{
		if (GetSelectedObjects().IsEmpty())
		{
			return SNullWidget::NullWidget;
		}

		FMenuBuilder MenuBuilder(true, CommandList);
		MenuBuilder.AddSubMenu(
			LOCALIZATION("AddChild"),
			FText::GetEmpty(),
			FNewMenuDelegate::CreateLambda([this](FMenuBuilder& SubMenuBuilder) {
				SubMenuBuilder.AddWidget(MakeAddChildMenu(), FText::GetEmpty(), true);
			})
		);
		MenuBuilder.AddMenuEntry(SceneViewCommands::Get().RenameObject);
		MenuBuilder.AddMenuEntry(SceneViewCommands::Get().DeleteObject);

		return MenuBuilder.MakeWidget();
	}

	void SSceneView::GetChildrenForItem(SceneObjectTreeItemPtr Item, TArray<SceneObjectTreeItemPtr>& OutChildren)
	{
		OutChildren = Item->Children;
	}

	TSharedRef<ITableRow> SSceneView::GenerateRowForItem(SceneObjectTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		return SNew(SSceneObjectRow, Item, this, TreeView.ToSharedRef(), OwnerTable);
	}

	void SSceneView::OnSelectionChanged(SceneObjectTreeItemPtr Item, ESelectInfo::Type SelectInfo)
	{
		if (bIgnoreSelectionChanged)
		{
			return;
		}

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		TArray<SceneObject*> OldSelectedObjects;
		for (const SceneObjectPtr& Obj : LastSelectedObjects)
		{
			if (Obj)
			{
				OldSelectedObjects.Add(Obj.Get());
			}
		}
		SceneObject* OldSelected = OldSelectedObjects.IsEmpty() ? nullptr : OldSelectedObjects[0];
		TArray<SceneObject*> SelectedObjects = GetSelectedObjects();
		SceneObject* NewSelected = SelectedObjects.IsEmpty() ? nullptr : SelectedObjects[0];

		if (NewSelected)
		{
			if (NewSelected != OldSelected)
			{
				if (OldSelected && !ShEditor->IsPropertyLocked())
				{
					ShEditor->RefreshProperty(true);
				}
			}
			ShEditor->ShowProperty(NewSelected);
		}
		else
		{
			if (OldSelected && !ShEditor->IsPropertyLocked())
			{
				ShEditor->RefreshProperty(true);
			}
		}

		int32 MatchingSelectionCount = 0;
		for (SceneObject* Obj : OldSelectedObjects)
		{
			if (SelectedObjects.Contains(Obj))
			{
				++MatchingSelectionCount;
			}
		}

		const bool bSelectionChanged = OldSelectedObjects.Num() != SelectedObjects.Num() ||
			MatchingSelectionCount != OldSelectedObjects.Num();
		if (bSelectionChanged && CurRender)
		{
			GetUndoManager().PushCommand(MakeShared<SelectionCommand>(this, OldSelectedObjects, SelectedObjects));
		}

		LastSelectedObjects.Empty();
		for (SceneObject* Obj : SelectedObjects)
		{
			if (Obj)
			{
				LastSelectedObjects.Add(Obj);
			}
		}
	}


	void SSceneView::BeginRenameSelected()
	{
		TArray<SceneObject*> SelectedObjects = GetSelectedObjects();
		SceneObject* SelectedObject = SelectedObjects.IsEmpty() ? nullptr : SelectedObjects[0];
		if (!SelectedObject)
		{
			return;
		}
		if (SceneObjectTreeItemPtr* ItemPtr = AllItems.Find(SelectedObject))
		{
			if ((*ItemPtr)->InlineTextBlock)
			{
				(*ItemPtr)->InlineTextBlock->EnterEditingMode();
			}
		}
	}

	void SSceneView::DeleteSelected()
	{
		if (!CurRender)
		{
			return;
		}

		TArray<SceneObjectTreeItemPtr> SelectedItems;
		TreeView->GetSelectedItems(SelectedItems);
		if (SelectedItems.IsEmpty())
		{
			return;
		}

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());

		// Only delete top-most selected objects; children will be removed recursively
		TArray<SceneObjectPtr> ToDelete;
		for (const SceneObjectTreeItemPtr& SelItem : SelectedItems)
		{
			if (!SelItem || !SelItem->Object)
			{
				continue;
			}

			bool bAncestorSelected = false;
			SceneObject* Ancestor = SelItem->Object->Parent.Get();
			while (Ancestor && !bAncestorSelected)
			{
				for (const SceneObjectTreeItemPtr& OtherItem : SelectedItems)
				{
					if (OtherItem && OtherItem->Object.Get() == Ancestor)
					{
						bAncestorSelected = true;
						break;
					}
				}
				Ancestor = Ancestor->Parent.Get();
			}

			if (!bAncestorSelected)
			{
				ToDelete.Add(SelItem->Object);
			}
		}

		SceneObject* PropertyObject = dynamic_cast<SceneObject*>(ShEditor->GetCurPropertyObject());
		bool bClearProperty = PropertyObject && ToDelete.ContainsByPredicate([PropertyObject](const SceneObjectPtr& O) {
			return O.Get() == PropertyObject;
		});

		if (bClearProperty)
		{
			ShEditor->RefreshProperty(true);
		}

		SelectObjectInternal(nullptr);
		if (CurRender)
		{
			SceneUndoManager::ScopedTransaction DeleteTransaction(GetUndoManager());
			for (const SceneObjectPtr& Obj : ToDelete)
			{
				int32 Index = CurRender->SceneObjects.IndexOfByPredicate([&Obj](const SceneObjectPtr& E) {
					return E.Get() == Obj.Get();
				});
				SceneObject* Parent = Obj->Parent.Get();
				int32 ParentChildIndex = Parent ? Parent->Children.IndexOfByKey(Obj.Get()) : INDEX_NONE;
				GetUndoManager().DoCommand(MakeShared<RemoveSceneObjectCommand>(this, CurRender, Obj, Index, Parent, ParentChildIndex));
			}
		}
		else
		{
			for (const SceneObjectPtr& Obj : ToDelete)
			{
				CurRender->RemoveSceneObject(Obj.Get());
			}
		}

		RefreshSceneItems();
		ShEditor->ForceRender();
	}

	void SSceneView::SelectObjects(const TArray<SceneObject*>& InObjects, bool bAdditive)
	{
		TArray<SceneObject*> OldSelectedObjects = GetSelectedObjects();
		TArray<SceneObject*> NewSelectedObjects = bAdditive ? OldSelectedObjects : TArray<SceneObject*>();
		for (SceneObject* Obj : InObjects)
		{
			if (Obj && !NewSelectedObjects.Contains(Obj))
			{
				NewSelectedObjects.Add(Obj);
			}
		}

		int32 MatchingSelectionCount = 0;
		for (SceneObject* Obj : OldSelectedObjects)
		{
			if (NewSelectedObjects.Contains(Obj))
			{
				++MatchingSelectionCount;
			}
		}

		const bool bSameSelection = OldSelectedObjects.Num() == NewSelectedObjects.Num() &&
			MatchingSelectionCount == OldSelectedObjects.Num();
		if (bSameSelection)
		{
			return;
		}

		SelectObjectsInternal(NewSelectedObjects);

		if (CurRender)
		{
			GetUndoManager().PushCommand(MakeShared<SelectionCommand>(this, OldSelectedObjects, NewSelectedObjects));
		}
	}

	void SSceneView::SelectObject(SceneObject* InObject, bool bAdditive)
	{
		TArray<SceneObject*> Objects;
		if (InObject)
		{
			Objects.Add(InObject);
		}
		SelectObjects(Objects, bAdditive);
	}

	TArray<SceneObject*> SSceneView::GetSelectedObjects() const
	{
		TArray<SceneObject*> Result;
		TArray<SceneObjectTreeItemPtr> Items;
		TreeView->GetSelectedItems(Items);
		for (auto& Item : Items)
		{
			if (Item && Item->Object)
			{
				Result.Add(Item->Object.Get());
			}
		}
		return Result;
	}

	bool SSceneView::IsObjectSelected(const SceneObject* Obj) const
	{
		if (!Obj)
		{
			return false;
		}

		TArray<SceneObject*> SelectedObjects = GetSelectedObjects();
		return SelectedObjects.Contains(const_cast<SceneObject*>(Obj));
	}

	void SSceneView::SelectObjectInternal(SceneObject* InObject)
	{
		TArray<SceneObject*> Objects;
		if (InObject)
		{
			Objects.Add(InObject);
		}
		SelectObjectsInternal(Objects);
	}

	void SSceneView::SelectObjectsInternal(const TArray<SceneObject*>& InObjects)
	{
		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		TArray<SceneObject*> SelectedObjects = GetSelectedObjects();
		SceneObject* OldSelected = SelectedObjects.IsEmpty() ? nullptr : SelectedObjects[0];
		SceneObject* NewSelected = InObjects.IsEmpty() ? nullptr : InObjects[0];

		if (OldSelected && OldSelected != NewSelected && !ShEditor->IsPropertyLocked())
		{
			ShEditor->RefreshProperty(true);
		}

		bIgnoreSelectionChanged = true;
		TreeView->ClearSelection();
		SceneObjectTreeItemPtr* FirstItemPtr = nullptr;
		for (SceneObject* Obj : InObjects)
		{
			if (SceneObjectTreeItemPtr* ItemPtr = AllItems.Find(Obj))
			{
				TreeView->SetItemSelection(*ItemPtr, true, ESelectInfo::Direct);
				if (!FirstItemPtr)
				{
					FirstItemPtr = ItemPtr;
				}
			}
		}
		if (FirstItemPtr)
		{
			TreeView->RequestScrollIntoView(*FirstItemPtr);
		}
		bIgnoreSelectionChanged = false;

		if (NewSelected)
		{
			ShEditor->ShowProperty(NewSelected);
		}

		LastSelectedObjects.Empty();
		for (SceneObject* Obj : InObjects)
		{
			if (Obj)
			{
				LastSelectedObjects.Add(Obj);
			}
		}
	}

	FReply SSceneView::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
	{
		if (CommandList->ProcessCommandBindings(InKeyEvent))
		{
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	void SSceneView::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
	{
		TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
		if (!DragDropOp)
		{
			return;
		}

		if (CurRender && HasDroppableModelAsset(DragDropOp))
		{
			DragDropOp->SetCursorOverride(EMouseCursor::GrabHand);
			return;
		}
		if (CurRender && DragDropOp->IsOfType<ShObjectDragDropOp>())
		{
			// Dropping on the background = detach from parent (make root) — always valid
			DragDropOp->SetCursorOverride(EMouseCursor::GrabHand);
			return;
		}

		DragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
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
			return HandleModelAssetDrop(this, DragDropOp) ? FReply::Handled() : FReply::Unhandled();
		}
		else if (CurRender && DragDropOp->IsOfType<ShObjectDragDropOp>())
		{
			// Dropped on the background — detach all selected items from parent (make root)
			TArray<SceneObjectTreeItemPtr> SelectedItems;
			TreeView->GetSelectedItems(SelectedItems);

			TArray<SceneObjectTreeItemPtr> ToDetach;
			for (const SceneObjectTreeItemPtr& SelItem : SelectedItems)
			{
				if (SelItem && SelItem->Object && SelItem->Object->Parent.IsValid())
				{
					ToDetach.Add(SelItem);
				}
			}
			if (!ToDetach.IsEmpty())
			{
				ReparentSceneObjects(ToDetach, nullptr);
			}
			return FReply::Handled();
		}
		return FReply::Unhandled();
	}

	void SSceneView::ReparentSceneObjects(const TArray<SceneObjectTreeItemPtr>& Items, SceneObject* NewParent)
	{
		if (!CurRender) return;

		auto& Mgr = GetUndoManager();

		// Build one command per object (snapshot old transform before any changes)
		TArray<TSharedPtr<SceneCommand>> Commands;
		for (const SceneObjectTreeItemPtr& DraggedItem : Items)
		{
			SceneObject* Obj = DraggedItem->Object.Get();
			if (Obj->Parent.Get() == NewParent) continue;

			Commands.Add(MakeShared<ReparentSceneObjectCommand>(
				this, DraggedItem->Object,
				Obj->Parent.Get(), Obj->Position, Obj->Rotation, Obj->Scale,
				NewParent
			));
		}

		if (Commands.IsEmpty()) return;

		SceneUndoManager::ScopedTransaction Transaction(Mgr);
		for (const auto& Cmd : Commands)
		{
			Mgr.DoCommand(Cmd);
		}

		// Expand the new parent so the newly-reparented child is visible
		if (NewParent)
		{
			if (SceneObjectTreeItemPtr* ParentItem = AllItems.Find(NewParent))
			{
				TreeView->SetItemExpansion(*ParentItem, true);
			}
		}
	}

	SceneUndoManager& SSceneView::GetUndoManager()
	{
		return UndoManager;
	}

	void SSceneView::Undo()
	{
		if (CurRender)
		{
			GetUndoManager().UndoAction();
		}
	}

	void SSceneView::Redo()
	{
		if (CurRender)
		{
			GetUndoManager().RedoAction();
		}
	}

	bool SSceneView::CanUndo() const
	{
		return CurRender && UndoManager.CanUndo();
	}

	bool SSceneView::CanRedo() const
	{
		return CurRender && UndoManager.CanRedo();
	}

}
