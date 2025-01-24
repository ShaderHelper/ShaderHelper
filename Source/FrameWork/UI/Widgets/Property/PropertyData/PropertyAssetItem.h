#include "GpuApi/GpuTexture.h"
#include "Editor/PreviewViewPort.h"
#include <Widgets/SViewport.h>
#include "UI/Widgets/AssetBrowser/AssetViewItem/AssetViewAssetItem.h"
#include "Editor/AssetEditor/AssetEditor.h"

namespace FW
{
    class SPropertyAsset : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SPropertyAsset)
        {}
            SLATE_ARGUMENT(ShObject*, Owner)
            SLATE_ARGUMENT(void* , AssetPtrRef);
            SLATE_ARGUMENT(MetaType* , AssetMetaType);
        SLATE_END_ARGS()
        
        void RefreshAssetView()
        {
            auto& AssetRef = *(AssetPtr<AssetObject>*)AssetPtrRef;
            if(AssetRef)
            {
                GpuTexture* Thumbnail = AssetRef->GetThumbnail();
                if(Thumbnail)
                {
                    auto ThumbnailViewport = MakeShared<PreviewViewPort>();
                    ThumbnailViewport->SetViewPortRenderTexture(Thumbnail);
                    Display->SetContent(SNew(SViewport).ViewportInterface(ThumbnailViewport));
                }
                else
                {
                    Display->SetContent(SNew(SImage).Image(AssetRef->GetImage()));
                }
            }
        }

        void Construct(const FArguments& InArgs)
        {
            Owner = InArgs._Owner;
            AssetPtrRef = InArgs._AssetPtrRef;
            AssetMetaType = InArgs._AssetMetaType;
            Display = SNew(SBox).WidthOverride(40).HeightOverride(40);
            
            auto PreviewBox = SNew(SBorder)
            .BorderImage_Lambda([this]{
                if(bRecognizedDragDrop) {
                    return FAppStyle::Get().GetBrush("Brushes.Highlight");
                }
                else if(bPreviewUnderMouse)
                {
                    return FAppStyle::Get().GetBrush("Brushes.Hover");
                }
                return FAppStyle::Get().GetBrush("Brushes.Panel");
            })
            .OnMouseMove_Lambda([this](const FGeometry&, const FPointerEvent&){
                bPreviewUnderMouse = true;
                return FReply::Handled();
            })
            .OnMouseDoubleClick_Lambda([this](const FGeometry&, const FPointerEvent&){
                if(auto& AssetRef = *(AssetPtr<AssetObject>*)AssetPtrRef)
                {
                    AssetOp::OpenAsset(AssetRef);
                }
                return FReply::Handled();
            })
            [
                SNew(SBorder)
                .BorderImage(FAppStyle::Get().GetBrush("Brushes.Black"))
                [
                    Display.ToSharedRef()
                ]
            ];
            PreviewBox->SetOnMouseLeave(FSimpleNoReplyPointerEventHandler::CreateLambda([this](const FPointerEvent&){
                bPreviewUnderMouse = false;
            }));
			PreviewBox->SetToolTipText(TAttribute<FText>::CreateLambda([this] {
				if (auto& AssetRef = *(AssetPtr<AssetObject>*)AssetPtrRef)
				{
					return FText::FromString(AssetRef->GetPath());
				}
				return FText{};
			}));
            
            RefreshAssetView();
            ChildSlot
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot()
                .AutoWidth()
                [
                    PreviewBox
                ]
                +SHorizontalBox::Slot()
                .VAlign(VAlign_Center)
                .AutoWidth()
                [
                    SNew(SVerticalBox)
                    +SVerticalBox::Slot()
                    .HAlign(HAlign_Center)
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .TextStyle(&FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallMinorText"))
                        .Text_Lambda([this]{
                            if(auto& AssetRef = *(AssetPtr<AssetObject>*)AssetPtrRef)
                            {
                                return AssetRef->ObjectName;
                            }
                            return FText::FromString("None");
                        })
                    ]
                    +SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .TextStyle(&FAppCommonStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallMinorText"))
                        .Text_Lambda([this]{
                            return FText::FromString("(" + GetRegisteredName(AssetMetaType) + ")");
                        })
                    ]
                ]
            ];
        }
        
        void OnDragEnter(FGeometry const& MyGeometry, FDragDropEvent const& DragDropEvent) override
        {
            TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
            if(DragDropOp->IsOfType<AssetViewItemDragDropOp>())
            {
                FString DropFilePath = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Path;
                MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
                if(DropAssetMetaType == AssetMetaType)
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
            TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
            bRecognizedDragDrop = false;
            DragDropOp->SetCursorOverride(TOptional<EMouseCursor::Type>());
        }
        
        FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
        {
            bRecognizedDragDrop = false;
            TSharedPtr<FDragDropOperation> DragDropOp = DragDropEvent.GetOperation();
            if(DragDropOp->IsOfType<AssetViewItemDragDropOp>())
            {
                FString DropFilePath = StaticCastSharedPtr<AssetViewItemDragDropOp>(DragDropOp)->Path;
                MetaType* DropAssetMetaType = GetAssetMetaType(DropFilePath);
                if(DropAssetMetaType == AssetMetaType)
                {
                    auto Asset = TSingleton<AssetManager>::Get().LoadAssetByPath<AssetObject>(DropFilePath);
                    auto& AssetRef = *(AssetPtr<AssetObject>*)AssetPtrRef;
                    if(AssetRef != Asset)
                    {
                        AssetRef = Asset;
                        RefreshAssetView();
                        Owner->GetOuterMost()->MarkDirty();
                    }
  
                }
            }
            return FReply::Handled();
        }

        
    private:
        bool bPreviewUnderMouse = false;
        bool bRecognizedDragDrop = false;
        ShObject* Owner;
        void* AssetPtrRef;
        MetaType* AssetMetaType;
        TSharedPtr<SBox> Display;
    };
    
    class PropertyAssetItem : public PropertyItemBase
    {
        MANUAL_RTTI_TYPE(PropertyAssetItem, PropertyItemBase)
    public:
        PropertyAssetItem(ShObject* InOwner, FString InName, MetaType* InAssetMetaType, void* InAssetPtrRef)
            : PropertyItemBase(InOwner, MoveTemp(InName))
            , AssetMetaType(InAssetMetaType)
            , AssetPtrRef(InAssetPtrRef)
        {}

        TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) override
        {
            auto Row = PropertyItemBase::GenerateWidgetForTableView(OwnerTable);
            auto ValueWidget = SNew(SPropertyAsset)
                .Owner(Owner)
                .AssetPtrRef(AssetPtrRef)
                .AssetMetaType(AssetMetaType);
            Item->AddWidget(MoveTemp(ValueWidget));
            return Row;
        }

    private:
        void* AssetPtrRef;
        MetaType* AssetMetaType;
    };
}
