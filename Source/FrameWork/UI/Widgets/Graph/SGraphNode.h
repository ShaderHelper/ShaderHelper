#pragma once
#include <Widgets/Text/SInlineEditableTextBlock.h>

namespace FW
{
	enum class PinDirection;

	class FRAMEWORK_API SGraphNode : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SGraphNode) : _NodeData(nullptr)
			{}
			SLATE_ARGUMENT(class GraphNode*, NodeData)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, class SGraphPanel* InOwnerPanel);
		void RebuildPins();
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
		virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const override;
		TSharedRef<SWidget> CreateContextMenu();
		void HandleRenameAction();
		void AddDep(SGraphNode* DependentNode);
		void RemoveDep(SGraphNode* DependentNode);
		Vector2D GetCollapsedPinConnectionPoint(const FGeometry& PanelGeometry, PinDirection Direction, bool bUsePaintSpaceGeometry) const;

	public:
		class GraphNode* NodeData;
		SGraphPanel* Owner;
		TArray<class SGraphPin*> Pins;

	private:
		void PopulatePinWidgets();
		bool IsOnResizeHandle(const FGeometry& MyGeometry, const FVector2D& ScreenSpacePosition) const;
		FVector2D MousePos;
		float ResizeStartWidth = 0.0f;
		float ResizeStartMouseX = 0.0f;
		bool bIsResizingWidth = false;
		TSharedPtr<SInlineEditableTextBlock> NodeTitleEditText;
		TSharedPtr<SWidget> NodeTitleWidget;
		TSharedPtr<SVerticalBox> PinContainer;
	};
}
