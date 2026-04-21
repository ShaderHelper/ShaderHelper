#include "CommonHeader.h"
#include "SceneUndoManager.h"
#include "SSceneView.h"
#include "App/App.h"
#include "Editor/ShaderHelperEditor.h"

#include <Serialization/MemoryWriter.h>
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
		SceneView->SelectObjectInternal(NewSelected.Get());
	}

	void SelectionCommand::Undo()
	{
		SceneView->SelectObjectInternal(OldSelected.Get());
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
		OwnerRender->InsertSceneObject(Index, Object);
		SceneView->RefreshSceneItems();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();
	}

	void AddSceneObjectCommand::Undo()
	{
		if (SceneView->GetSelectedObject() == Object.Get())
		{
			SceneView->SelectObjectInternal(nullptr);
		}

		OwnerRender->RemoveSceneObject(Object.Get());
		SceneView->RefreshSceneItems();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();
	}
	
	void RemoveSceneObjectCommand::Do()
	{
		if (SceneView->GetSelectedObject() == Object.Get())
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
		OwnerRender->InsertSceneObject(Index, Object);
		SceneView->RefreshSceneItems();

		auto ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		ShEditor->ForceRender();
	}
}
