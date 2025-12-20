#pragma once

#include <Widgets/Views/STileView.h>

namespace FW
{
    
    class AssetViewItemDragDropOp : public FDragDropOperation
    {
    public:
        DRAG_DROP_OPERATOR_TYPE(AssetViewItemDragDropOp, FDragDropOperation)
        
        static TSharedRef<AssetViewItemDragDropOp> New(const TArray<FString>& InPaths)
        {
            TSharedRef<AssetViewItemDragDropOp> Operation = MakeShareable(new AssetViewItemDragDropOp(InPaths));
            Operation->MouseCursor = EMouseCursor::GrabHandClosed;
            Operation->Construct();
            return Operation;
        }
        
        TArray<FString> Paths;
    protected:
        AssetViewItemDragDropOp(const TArray<FString>& InPaths)
            : Paths(InPaths)
        {}
    };
    
	class AssetViewItem
	{
        MANUAL_RTTI_BASE_TYPE()
	public:
		AssetViewItem(STileView<TSharedRef<AssetViewItem>>* InOwner, const FString& InPath) : Owner(InOwner), Path(InPath) {}
		virtual ~AssetViewItem() = default;
		virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) = 0;
		FString GetPath() const { return Path; }

	protected:
		STileView<TSharedRef<AssetViewItem>>* Owner;
		FString Path;
	};
}
