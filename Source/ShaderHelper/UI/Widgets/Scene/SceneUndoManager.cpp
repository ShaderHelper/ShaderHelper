#include "CommonHeader.h"
#include "SceneUndoManager.h"
#include "SSceneView.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

#include <Serialization/MemoryReader.h>

using namespace FW;

namespace SH
{
	void SceneUndoManager::UndoAction()
	{
		if (!UndoStack.IsEmpty())
		{
			auto State = UndoStack.Pop();
			for (int32 Index = State.Commands.Num() - 1; Index >= 0; Index--)
			{
				State.Commands[Index]->Undo();
			}
			RedoStack.Add(MoveTemp(State));
		}
	}

	void SceneUndoManager::RedoAction()
	{
		if (!RedoStack.IsEmpty())
		{
			auto State = RedoStack.Pop();
			for (const auto& Command : State.Commands)
			{
				Command->Do();
			}
			UndoStack.Add(MoveTemp(State));
		}
	}

	void SelectionCommand::Do()
	{
		TArray<SceneObject*> ResolvedSelection;
		for (const auto& Obj : NewSelection)
		{
			if (Obj)
			{
				ResolvedSelection.Add(Obj.Get());
			}
		}
		SceneView->SelectObjectsInternal(ResolvedSelection);
	}

	void SelectionCommand::Undo()
	{
		TArray<SceneObject*> ResolvedSelection;
		for (const auto& Obj : OldSelection)
		{
			if (Obj)
			{
				ResolvedSelection.Add(Obj.Get());
			}
		}
		SceneView->SelectObjectsInternal(ResolvedSelection);
	}

	void TransformCommand::Do()
	{
		Object->Position = NewPosition;
		Object->Rotation = NewRotation;
		Object->Scale = NewScale;
		Object->GetOuterMost()->MarkDirty();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (ShEditor->GetCurPropertyObject() == Object.Get())
		{
			ShEditor->RefreshProperty();
		}
		ShEditor->ForceRender();
	}

	void TransformCommand::Undo()
	{
		Object->Position = OldPosition;
		Object->Rotation = OldRotation;
		Object->Scale = OldScale;
		Object->GetOuterMost()->MarkDirty();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (ShEditor->GetCurPropertyObject() == Object.Get())
		{
			ShEditor->RefreshProperty();
		}
		ShEditor->ForceRender();
	}

	void PropertyChangeCommand::Do()
	{
		ApplyData(NewData);
	}

	void PropertyChangeCommand::Undo()
	{
		ApplyData(OldData);
	}

	void PropertyChangeCommand::ApplyData(const TArray<uint8>& Data)
	{
		TArray<uint8> DataCopy = Data;
		FMemoryReader Ar(DataCopy);
		Object->Serialize(Ar);

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		Object->GetOuterMost()->MarkDirty();
		if (ShEditor->GetCurPropertyObject() == Object.Get())
		{
			ShEditor->RefreshProperty();
		}
		ShEditor->ForceRender();
	}

	void AddSceneObjectCommand::Do()
	{
		OwnerRender->InsertSceneObject(Index, Object, ParentObject, ParentChildIndex);
		SceneView->RefreshSceneItems();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();
	}

	void AddSceneObjectCommand::Undo()
	{
		if (SceneView->IsObjectSelected(Object.Get()))
		{
			SceneView->SelectObjectInternal(nullptr);
		}

		ParentObject = Object->Parent.Get();
		if (ParentObject)
		{
			ParentChildIndex = ParentObject->Children.IndexOfByKey(Object.Get());
		}

		OwnerRender->RemoveSceneObject(Object.Get());
		SceneView->RefreshSceneItems();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();
	}

	RemoveSceneObjectCommand::RemoveSceneObjectCommand(SSceneView* InSceneView, Render* InRender, FW::ObjectPtr<SceneObject> InObject,
		int32 InIndex, SceneObject* InParent, int32 InParentChildIndex)
		: SceneCommand(InSceneView)
		, OwnerRender(InRender)
		, Object(MoveTemp(InObject))
		, Index(InIndex)
		, ParentObject(InParent)
		, ParentChildIndex(InParentChildIndex)
	{
		bWasExpanded = WasObjectExpanded(Object.Get());
		CaptureRemovedChildren(Object.Get());
		RemovedChildren.Sort([](const RemovedSceneObjectState& A, const RemovedSceneObjectState& B) {
			return A.Index < B.Index;
		});
	}

	bool RemoveSceneObjectCommand::WasObjectExpanded(SceneObject* InObject) const
	{
		if (!InObject)
		{
			return false;
		}

		if (SceneObjectTreeItemPtr* ItemPtr = SceneView->AllItems.Find(InObject))
		{
			return SceneView->TreeView->IsItemExpanded(*ItemPtr);
		}

		return false;
	}

	void RemoveSceneObjectCommand::CaptureRemovedChildren(SceneObject* InObject)
	{
		if (!InObject)
		{
			return;
		}

		for (SceneObject* Child : InObject->Children)
		{
			if (!Child)
			{
				continue;
			}

			RemovedSceneObjectState& State = RemovedChildren.AddDefaulted_GetRef();
			State.Object = FW::ObjectPtr<SceneObject>(Child);
			State.Index = OwnerRender->SceneObjects.IndexOfByPredicate([Child](const ObjectPtr<SceneObject>& Element) {
				return Element.Get() == Child;
			});
			State.ParentObject = Child->Parent.Get();
			State.ParentChildIndex = State.ParentObject ? State.ParentObject->Children.IndexOfByKey(Child) : INDEX_NONE;
			State.bWasExpanded = WasObjectExpanded(Child);

			CaptureRemovedChildren(Child);
		}
	}

	void RemoveSceneObjectCommand::RestoreExpansionState() const
	{
		if (bWasExpanded)
		{
			if (SceneObjectTreeItemPtr* ItemPtr = SceneView->AllItems.Find(Object.Get()))
			{
				SceneView->TreeView->SetItemExpansion(*ItemPtr, true);
			}
		}

		for (const RemovedSceneObjectState& State : RemovedChildren)
		{
			if (!State.bWasExpanded)
			{
				continue;
			}

			if (SceneObjectTreeItemPtr* ItemPtr = SceneView->AllItems.Find(State.Object.Get()))
			{
				SceneView->TreeView->SetItemExpansion(*ItemPtr, true);
			}
		}
	}

	bool RemoveSceneObjectCommand::IsAnyRemovedObjectSelected() const
	{
		if (SceneView->IsObjectSelected(Object.Get()))
		{
			return true;
		}

		for (const RemovedSceneObjectState& State : RemovedChildren)
		{
			if (SceneView->IsObjectSelected(State.Object.Get()))
			{
				return true;
			}
		}

		return false;
	}

	void RemoveSceneObjectCommand::Do()
	{
		if (IsAnyRemovedObjectSelected())
		{
			SceneView->SelectObjectInternal(nullptr);
		}

		OwnerRender->RemoveSceneObject(Object.Get());
		SceneView->RefreshSceneItems();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();
	}

	void RemoveSceneObjectCommand::Undo()
	{
		OwnerRender->InsertSceneObject(Index, Object, ParentObject, ParentChildIndex);
		for (const RemovedSceneObjectState& State : RemovedChildren)
		{
			OwnerRender->InsertSceneObject(State.Index, State.Object, State.ParentObject, State.ParentChildIndex);
		}
		SceneView->RefreshSceneItems();
		RestoreExpansionState();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();
	}

	void RenameSceneObjectCommand::Do()
	{
		Object->ObjectName = NewName;
		Object->GetOuterMost()->MarkDirty();
		SceneView->RefreshSceneItems();
	}

	void RenameSceneObjectCommand::Undo()
	{
		Object->ObjectName = OldName;
		Object->GetOuterMost()->MarkDirty();
		SceneView->RefreshSceneItems();
	}

	void ReparentSceneObjectCommand::Do()
	{
		Object->SetParent(NewParent);
		NewPos = Object->Position;
		NewRot = Object->Rotation;
		NewScale = Object->Scale;

		Object->GetOuterMost()->MarkDirty();
		SceneView->RefreshSceneItems();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();
	}

	void ReparentSceneObjectCommand::Undo()
	{
		// Directly restore old parent links and exact transform
		if (Object->Parent.IsValid())
		{
			Object->Parent->Children.Remove(Object.Get());
			Object->Parent.Reset();
		}
		if (OldParent)
		{
			Object->Parent = OldParent;
			OldParent->Children.Add(Object.Get());
		}
		Object->Position = OldPos;
		Object->Rotation = OldRot;
		Object->Scale = OldScale;

		Object->GetOuterMost()->MarkDirty();
		SceneView->RefreshSceneItems();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();
	}
}
