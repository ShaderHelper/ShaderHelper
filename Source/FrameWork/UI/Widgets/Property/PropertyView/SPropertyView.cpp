#include "CommonHeader.h"
#include "SPropertyView.h"
#include "UI/Widgets/Misc/MiscWidget.h"
#include "AssetObject/AssetObject.h"

namespace FW
{
    void SPropertyView::Construct(const FArguments& InArgs)
    {
        SetObjectData(InArgs._ObjectData);
    }

    void SPropertyView::SetObjectData(ShObject* InObjectData)
    {
        if(!InObjectData) return;
        if(Locked || InObjectData == ObjectData)
        {
            return;
        }
            
        ObjectData = InObjectData;
        
        TSharedPtr<SBorder> PropertyContent;
        auto Content = SNew(SVerticalBox)
        .Visibility_Lambda([this]{
            if(ObjectData.IsValid())
            {
                return EVisibility::Visible;
            }
            else
            {
                Locked = false;
                return EVisibility::Collapsed;
            }
            
        })
        +SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SBorder)
            .BorderImage(FAppStyle::Get().GetBrush("Brushes.Panel"))
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                .Padding(4,4,4,0)
                .FillWidth(1.0f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(STextBlock)
                        .OverflowPolicy(ETextOverflowPolicy::Ellipsis)
                        .ToolTipText_Lambda([this] {
                            return FText::FromString(ObjectData->ObjectName.ToString());
                        })
                        .Text_Lambda([this]{
                            return FText::FromString(ObjectData->ObjectName.ToString());
                        })
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .VAlign(VAlign_Center)
                    .Padding(6, 0, 0, 0)
                    [
                        SNew(STextBlock)
                        .Text_Lambda([this]{
                            FString TypeName = GetRegisteredName(ObjectData->DynamicMetaType());
                            return FText::FromString(FString::Printf(TEXT("(%s)"), *TypeName));
                        })
                    ]
                ]
                +SHorizontalBox::Slot()
                .AutoWidth()
                .HAlign(HAlign_Right)
                .VAlign(VAlign_Top)
                [
                    SNew(SIconButton)
                    .Icon_Lambda([this]{
                        if(Locked) {
                            return FAppStyle::Get().GetBrush("Icons.Lock");
                        }
                        return FAppStyle::Get().GetBrush("Icons.Unlock");
                    })
                    .OnClicked_Lambda([this] {
                        Locked = !Locked;
                        return FReply::Handled();
                    })
                ]
            ]
            
        ]
        +SVerticalBox::Slot()
        .AutoHeight()
        .HAlign(HAlign_Right)
        .Padding(0,4,4,4)
        [
            SNew(SButton)
            .Text(LOCALIZATION("Save"))
            .IsEnabled_Lambda([this] {
                return ObjectData.IsValid()
                    && ObjectData->DynamicMetaType()->IsDerivedFrom<AssetObject>()
                    && static_cast<AssetObject*>(ObjectData.Get())->IsDirty();
            })
            .OnClicked_Lambda([this] {
                static_cast<AssetObject*>(ObjectData.Get())->Save();
                return FReply::Handled();
            })
            .Visibility_Lambda([this] {
                return (ObjectData.IsValid() && ObjectData->DynamicMetaType()->IsDerivedFrom<AssetObject>())
                    ? EVisibility::Visible : EVisibility::Collapsed;
            })
        ]
        +SVerticalBox::Slot()
        [
            SAssignNew(PropertyContent, SBorder)
            .BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
            .Padding(4.0)
        ];
        PropertyDatas = ObjectData->GeneratePropertyDatas();
        if(!PropertyDatas.IsEmpty())
        {
            PropertyContent->SetContent(
                SAssignNew(PropertyTree, STreeView<TSharedRef<PropertyData>>)
                .IsEnabled_Lambda([this] {
                    return !(ObjectData.IsValid()
                        && ObjectData->DynamicMetaType()->IsDerivedFrom<AssetObject>()
                        && static_cast<AssetObject*>(ObjectData.Get())->IsBuiltInAsset());
                })
                .TreeItemsSource(&PropertyDatas)
                .OnGetChildren_Lambda([](TSharedRef<PropertyData> InTreeNode, TArray<TSharedRef<PropertyData>>& OutChildren) {
                    InTreeNode->GetChildren(OutChildren);
                })
                .OnGenerateRow_Lambda([](TSharedRef<PropertyData> InTreeNode, const TSharedRef<STableViewBase>& OwnerTable) {
                    return InTreeNode->GenerateWidgetForTableView(OwnerTable);
                })
                .OnExpansionChanged_Lambda([](TSharedRef<PropertyData> InData, bool bExpanded){
                    InData->Expanded = bExpanded;
                })
                .SelectionMode(ESelectionMode::None)
            );
            
            for(const auto& Data : PropertyDatas)
            {
                TryExpandItemRecursively(Data);
            }
        }
        
        ChildSlot.AttachWidget(Content);
    }
}
