#pragma once
#include "AssetObject/Render/SceneObject.h"
#include "AssetObject/Render/Render.h"
#include "SceneUndoManager.h"

class SInlineEditableTextBlock;

namespace SH
{
	using SceneObjectPtr = FW::ObjectPtr<SceneObject>;

	struct SceneObjectListItem
	{
		explicit SceneObjectListItem(SceneObjectPtr InObject)
			: Object(MoveTemp(InObject))
		{
		}

		SceneObjectPtr Object;
		TSharedPtr<SInlineEditableTextBlock> InlineTextBlock;
	};

	using SceneObjectListItemPtr = TSharedPtr<SceneObjectListItem>;

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
		SceneObject* GetSelectedObject() const { return SelectedObject.Get(); }
		void SelectObject(SceneObject* InObject);
		void RefreshSceneItems();

		SceneUndoManager* GetUndoManager();
		void Undo();
		void Redo();
		bool CanUndo() const;
		bool CanRedo() const;

		void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
		void OnDragLeave(const FDragDropEvent& DragDropEvent) override;
		FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;

	private:
		void SelectObjectInternal(SceneObject* InObject);
		TSharedRef<ITableRow> GenerateRowForItem(SceneObjectListItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
		TSharedRef<SWidget> MakeAddMenu();
		TSharedPtr<SWidget> CreateContextMenu();
		void OnSelectionChanged(SceneObjectListItemPtr Item, ESelectInfo::Type SelectInfo);
		void DeleteSelected();
		void BeginRenameSelected();

	private:
		Render* CurRender = nullptr;
		TArray<SceneObjectListItemPtr> SceneItems;
		TSharedPtr<SListView<SceneObjectListItemPtr>> ListView;
		SceneObjectPtr SelectedObject;

		TMap<Render*, TUniquePtr<SceneUndoManager>> UndoManagers;
		bool bIgnoreSelectionChanged = false;
	};
}
