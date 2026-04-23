#pragma once
#include "PropertyItem.h"

namespace FW
{
	class SPropertyObject : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SPropertyObject)
		{}
			SLATE_ARGUMENT(PropertyData*, Owner)
			SLATE_ARGUMENT(ShObject*, OuterObject)
			SLATE_ARGUMENT(MetaType*, ObjectMetaType)
			SLATE_ARGUMENT(bool, ReadOnly)
			SLATE_ARGUMENT(TFunction<ShObject*()>, GetObject)
			SLATE_ARGUMENT(TFunction<void(ShObject*)>, SetObject)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Owner = InArgs._Owner;
			OuterObject = InArgs._OuterObject;
			ObjectMetaType = InArgs._ObjectMetaType;
			ReadOnly = InArgs._ReadOnly;
			GetObject = InArgs._GetObject;
			SetObject = InArgs._SetObject;

			ChildSlot
			[
				SNew(SBorder)
				.BorderImage_Lambda([this] {
					return bRecognizedDragDrop ? FAppStyle::Get().GetBrush("Brushes.Highlight") : FAppStyle::Get().GetBrush("Brushes.Input");
				})
				.Padding(FMargin(8.0f, 4.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this] {
							if (GetObject)
							{
								if (ShObject* Object = GetObject())
								{
									return Object->ObjectName;
								}
							}
							return FText::FromString("None");
						})
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(SIconButton)
						.Icon(FAppCommonStyle::Get().GetBrush("Icons.Close"))
						.Visibility_Lambda([this] {
							return !ReadOnly && GetObject && GetObject() ? EVisibility::Visible : EVisibility::Collapsed;
						})
						.OnClicked_Lambda([this] {
							AssignObject(nullptr);
							return FReply::Handled();
						})
					]
				]
			];
		}

		void OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent) override
		{
			if (ReadOnly)
			{
				return;
			}

			TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
			if (DragDropOp->IsOfType<ShObjectDragDropOp>())
			{
				ShObject* DroppedObject = StaticCastSharedPtr<ShObjectDragDropOp>(DragDropOp)->Object;
				if (CanAcceptObject(DroppedObject))
				{
					bRecognizedDragDrop = true;
					DragDropOp->SetCursorOverride(EMouseCursor::GrabHand);
				}
				else
				{
					DragDropOp->SetCursorOverride(EMouseCursor::SlashedCircle);
				}
			}
		}

		void OnDragLeave(FDragDropEvent const& DragDropEvent) override
		{
			bRecognizedDragDrop = false;
			if (TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation())
			{
				DragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
			}
		}

		FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
		{
			bRecognizedDragDrop = false;
			if (ReadOnly)
			{
				return FReply::Handled();
			}

			TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
			if (DragDropOp->IsOfType<ShObjectDragDropOp>())
			{
				AssignObject(StaticCastSharedPtr<ShObjectDragDropOp>(DragDropOp)->Object);
				DragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
			}
			return FReply::Handled();
		}

	private:
		bool CanAcceptObject(ShObject* InObject) const
		{
			if (!InObject || !ObjectMetaType)
			{
				return false;
			}

			for (MetaType* CurMetaType = InObject->DynamicMetaType(); CurMetaType; CurMetaType = CurMetaType->GetBaseClass())
			{
				if (CurMetaType == ObjectMetaType)
				{
					return true;
				}
			}

			return false;
		}

		void AssignObject(ShObject* NewObject)
		{
			if (!Owner || !OuterObject || !SetObject)
			{
				return;
			}

			ShObject* CurrentObject = GetObject ? GetObject() : nullptr;
			if (CurrentObject == NewObject)
			{
				return;
			}

			if (NewObject && !CanAcceptObject(NewObject))
			{
				return;
			}

			if (!OuterObject->CanChangeProperty(Owner))
			{
				return;
			}

			Owner->BeginEdit();
			SetObject(NewObject);
			OuterObject->PostPropertyChanged(Owner);
			Owner->EndEdit();
		}

		bool bRecognizedDragDrop = false;
		PropertyData* Owner = nullptr;
		ShObject* OuterObject = nullptr;
		MetaType* ObjectMetaType = nullptr;
		bool ReadOnly = false;
		TFunction<ShObject*()> GetObject;
		TFunction<void(ShObject*)> SetObject;
	};

	class PropertyObjectItem : public PropertyItemBase
	{
		MANUAL_RTTI_TYPE(PropertyObjectItem, PropertyItemBase)
	public:
		PropertyObjectItem(ShObject* InOwner, FText InName, MetaType* InObjectMetaType, TFunction<ShObject*()> InGetObject, TFunction<void(ShObject*)> InSetObject, bool InReadOnly = false)
			: PropertyItemBase(InOwner, MoveTemp(InName))
			, ObjectMetaType(InObjectMetaType)
			, GetObject(MoveTemp(InGetObject))
			, SetObject(MoveTemp(InSetObject))
			, ReadOnly(InReadOnly)
		{}

		TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
		{
			auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
			Item->AddWidget(
				SNew(SPropertyObject)
				.Owner(this)
				.OuterObject(Owner)
				.ObjectMetaType(ObjectMetaType)
				.ReadOnly(ReadOnly)
				.GetObject(GetObject)
				.SetObject(SetObject)
			);
			return Row;
		}

	private:
		MetaType* ObjectMetaType = nullptr;
		TFunction<ShObject*()> GetObject;
		TFunction<void(ShObject*)> SetObject;
		bool ReadOnly = false;
	};
}
