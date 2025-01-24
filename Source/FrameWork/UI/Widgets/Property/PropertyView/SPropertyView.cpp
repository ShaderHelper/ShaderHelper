#include "CommonHeader.h"
#include "SPropertyView.h"
#include "UI/Widgets/Misc/SIconButton.h"

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
                .Padding(4,4,0,0)
                .AutoWidth()
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]{
                        FString TypeName = GetRegisteredName(ObjectData->DynamicMetaType());
                        return FText::FromString(ObjectData->ObjectName.ToString() + FString::Printf(TEXT(" (%s)"), *TypeName));
                    })
                ]
                +SHorizontalBox::Slot()
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
        [
            SAssignNew(PropertyContent, SBorder)
            .BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
            .Padding(4.0)
        ];
        PropertyDatas = ObjectData->GetPropertyDatas();
        if(PropertyDatas && !PropertyDatas->IsEmpty())
        {
            PropertyContent->SetContent(
                SAssignNew(PropertyTree, STreeView<TSharedRef<PropertyData>>)
                .TreeItemsSource(PropertyDatas)
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
            
            for(const auto& Data : *PropertyDatas)
            {
                TryExpandItemRecursively(Data);
            }
        }
        
        ChildSlot.AttachWidget(Content);
    }
}
