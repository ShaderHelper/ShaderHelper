#pragma once

namespace FW
{
    
    class AssetViewItemDragDropOp : public FDragDropOperation
    {
    public:
        DRAG_DROP_OPERATOR_TYPE(AssetViewItemDragDropOp, FDragDropOperation)
        
        static TSharedRef<AssetViewItemDragDropOp> New(const FString& InPath)
        {
            TSharedRef<AssetViewItemDragDropOp> Operation = MakeShareable(new AssetViewItemDragDropOp(InPath));
            Operation->MouseCursor = EMouseCursor::GrabHandClosed;
            Operation->Construct();
            return Operation;
        }
        
        FString Path;
    protected:
        AssetViewItemDragDropOp(const FString& InPath)
            : Path(InPath)
        {}
    };
    
	class AssetViewItem
	{
	public:
		MANUAL_RTTI_BASE_TYPE()

		AssetViewItem(const FString& InPath) : Path(InPath) {}
		virtual ~AssetViewItem() = default;
		virtual TSharedRef<ITableRow> GenerateWidgetForTableView(const TSharedRef<STableViewBase>& OwnerTable) = 0;
		FString GetPath() const { return Path; }

	protected:
		FString Path;
	};
}
