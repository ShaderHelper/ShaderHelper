#pragma once
#include "AssetObject/Render/SceneObject.h"
#include "AssetObject/Render/Render.h"

#include <optional>

namespace SH
{
	class SSceneView;

	class SceneCommand
	{
	public:
		SceneCommand(SSceneView* InSceneView) : SceneView(InSceneView) {}
		virtual ~SceneCommand() = default;
		virtual void Do() {}
		virtual void Undo() {}

	protected:
		SSceneView* SceneView;
	};

	struct SceneUndoState
	{
		TArray<TSharedPtr<SceneCommand>> Commands;
	};

	class SceneUndoManager
	{
	public:
		struct ScopedTransaction
		{
			ScopedTransaction(SceneUndoManager* InManager)
				: Manager(InManager)
			{
				if (!Manager->CurrentTransaction)
				{
					Manager->CurrentTransaction = Transaction{};
				}
			}

			~ScopedTransaction()
			{
				if (Manager->CurrentTransaction.value().Commands.Num() > 0)
				{
					Manager->UndoStack.Emplace(Manager->CurrentTransaction.value().Commands);
					Manager->RedoStack.Empty();
				}
				Manager->CurrentTransaction.reset();
			}

			SceneUndoManager* Manager;
		};

		struct Transaction
		{
			TArray<TSharedPtr<SceneCommand>> Commands;
		};

		void DoCommand(TSharedPtr<SceneCommand> Command)
		{
			if (CurrentTransaction)
			{
				CurrentTransaction.value().Commands.Add(Command);
			}
			Command->Do();
		}

		void PushCommand(TSharedPtr<SceneCommand> Command)
		{
			SceneUndoState State;
			State.Commands.Add(MoveTemp(Command));
			UndoStack.Add(MoveTemp(State));
			RedoStack.Empty();
		}

		void UndoAction();
		void RedoAction();

		bool CanUndo() const { return !UndoStack.IsEmpty(); }
		bool CanRedo() const { return !RedoStack.IsEmpty(); }

		void Clear()
		{
			UndoStack.Empty();
			RedoStack.Empty();
			CurrentTransaction.reset();
		}

	private:
		std::optional<Transaction> CurrentTransaction;
		TArray<SceneUndoState> UndoStack;
		TArray<SceneUndoState> RedoStack;
	};

	class SelectionCommand : public SceneCommand
	{
	public:
		SelectionCommand(SSceneView* InSceneView, SceneObject* InOldSelected, SceneObject* InNewSelected)
			: SceneCommand(InSceneView)
			, OldSelected(InOldSelected)
			, NewSelected(InNewSelected)
		{}

		void Do() override;
		void Undo() override;

	private:
		FW::ObjectPtr<SceneObject> OldSelected;
		FW::ObjectPtr<SceneObject> NewSelected;
	};

	class TransformCommand : public SceneCommand
	{
	public:
		TransformCommand(SSceneView* InSceneView, FW::ObjectPtr<SceneObject> InObject,
			const FW::Vector3f& InOldPos, const FW::Vector3f& InOldRot, const FW::Vector3f& InOldScale,
			const FW::Vector3f& InNewPos, const FW::Vector3f& InNewRot, const FW::Vector3f& InNewScale)
			: SceneCommand(InSceneView)
			, Object(MoveTemp(InObject))
			, OldPosition(InOldPos), OldRotation(InOldRot), OldScale(InOldScale)
			, NewPosition(InNewPos), NewRotation(InNewRot), NewScale(InNewScale)
		{}

		void Do() override;
		void Undo() override;

	private:
		FW::ObjectPtr<SceneObject> Object;
		FW::Vector3f OldPosition, OldRotation, OldScale;
		FW::Vector3f NewPosition, NewRotation, NewScale;
	};

	class PropertyChangeCommand : public SceneCommand
	{
	public:
		PropertyChangeCommand(SSceneView* InSceneView, FW::ObjectPtr<SceneObject> InObject,
			TArray<uint8>&& InOldData, TArray<uint8>&& InNewData)
			: SceneCommand(InSceneView)
			, Object(MoveTemp(InObject))
			, OldData(MoveTemp(InOldData))
			, NewData(MoveTemp(InNewData))
		{}

		void Do() override;
		void Undo() override;

	private:
		void ApplyData(const TArray<uint8>& Data);

		FW::ObjectPtr<SceneObject> Object;
		TArray<uint8> OldData;
		TArray<uint8> NewData;
	};

	class AddSceneObjectCommand : public SceneCommand
	{
	public:
		AddSceneObjectCommand(SSceneView* InSceneView, Render* InRender, FW::ObjectPtr<SceneObject> InObject, int32 InIndex)
			: SceneCommand(InSceneView)
			, OwnerRender(InRender)
			, Object(MoveTemp(InObject))
			, Index(InIndex)
		{}

		void Do() override;
		void Undo() override;

	private:
		Render* OwnerRender;
		FW::ObjectPtr<SceneObject> Object;
		int32 Index;
	};

	class RemoveSceneObjectCommand : public SceneCommand
	{
	public:
		RemoveSceneObjectCommand(SSceneView* InSceneView, Render* InRender, FW::ObjectPtr<SceneObject> InObject, int32 InIndex)
			: SceneCommand(InSceneView)
			, OwnerRender(InRender)
			, Object(MoveTemp(InObject))
			, Index(InIndex)
		{}

		void Do() override;
		void Undo() override;

	private:
		Render* OwnerRender;
		FW::ObjectPtr<SceneObject> Object;
		int32 Index;
	};

	class RenameSceneObjectCommand : public SceneCommand
	{
	public:
		RenameSceneObjectCommand(SSceneView* InSceneView, FW::ObjectPtr<SceneObject> InObject, const FText& InOldName, const FText& InNewName)
			: SceneCommand(InSceneView)
			, Object(MoveTemp(InObject))
			, OldName(InOldName)
			, NewName(InNewName)
		{}

		void Do() override;
		void Undo() override;

	private:
		FW::ObjectPtr<SceneObject> Object;
		FText OldName;
		FText NewName;
	};
}
