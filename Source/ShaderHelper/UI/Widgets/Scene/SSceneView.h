#pragma once
#include "AssetObject/Render/SceneObject.h"
#include "AssetObject/Render/Render.h"
#include "SceneUndoManager.h"

class SInlineEditableTextBlock;

namespace SH
{
	using SceneObjectPtr = FW::ObjectPtr<SceneObject>;

	struct SceneObjectTreeItem
	{
		explicit SceneObjectTreeItem(SceneObjectPtr InObject)
			: Object(MoveTemp(InObject))
		{
		}

		SceneObjectPtr Object;
		TSharedPtr<SInlineEditableTextBlock> InlineTextBlock;
		TArray<TSharedPtr<SceneObjectTreeItem>> Children;
	};

	using SceneObjectTreeItemPtr = TSharedPtr<SceneObjectTreeItem>;

	enum class GizmoMode : int32
	{
		Move = 0,
		Rotate = 1,
		Scale = 2,
	};

	enum class GizmoSpace : int32
	{
		Global = 0,
		Local = 1,
	};

	class SSceneView : public SCompoundWidget
	{
		friend class SelectionCommand;
		friend class AddSceneObjectCommand;
		friend class RemoveSceneObjectCommand;

	public:
		SLATE_BEGIN_ARGS(SSceneView) {}
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);
		void SetRender(Render* InRender);
		Render* GetRender() const { return CurRender; }
		TArray<SceneObject*> GetSelectedObjects() const;
		void SelectObjects(const TArray<SceneObject*>& InObjects, bool bAdditive = false);
		void SelectObject(SceneObject* InObject, bool bAdditive = false);
		void RefreshSceneItems();
		void DeleteSelected();
		void BeginRenameSelected();

		SceneUndoManager& GetUndoManager();
		void Undo();
		void Redo();
		bool CanUndo() const;
		bool CanRedo() const;

		void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
		FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

		bool IsObjectSelected(const SceneObject* Obj) const;
		void ReparentSceneObjects(const TArray<SceneObjectTreeItemPtr>& Items, SceneObject* NewParent);

	private:
		void SelectObjectsInternal(const TArray<SceneObject*>& InObjects);
		void SelectObjectInternal(SceneObject* InObject);
		TSharedRef<ITableRow> GenerateRowForItem(SceneObjectTreeItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
		void GetChildrenForItem(SceneObjectTreeItemPtr Item, TArray<SceneObjectTreeItemPtr>& OutChildren);
		TSharedRef<SWidget> MakeAddMenu();
		TSharedRef<SWidget> MakeAddChildMenu();
		TSharedPtr<SWidget> CreateContextMenu();
		void OnSelectionChanged(SceneObjectTreeItemPtr Item, ESelectInfo::Type SelectInfo);

		template<typename T>
		void DoAddSceneObject(SceneObject* InParent = nullptr);

	private:
		Render* CurRender = nullptr;
		TArray<SceneObjectTreeItemPtr> RootItems;
		TMap<SceneObject*, SceneObjectTreeItemPtr> AllItems;
		TSharedPtr<STreeView<SceneObjectTreeItemPtr>> TreeView;

		SceneUndoManager UndoManager;
		bool bIgnoreSelectionChanged = false;
		TArray<SceneObjectPtr> LastSelectedObjects;
		TSharedPtr<FUICommandList> CommandList;
	};
}

