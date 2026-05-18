#pragma once
#include "Debugger/DebuggableObject.h"

class SButton;

namespace SH
{
	class SComputeDebuggerViewport : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SComputeDebuggerViewport) {}
		SLATE_END_ARGS()
		void Construct(const FArguments& InArgs);
		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

		void SetComputeDebugInfo(const FW::Vector3u& InThreadGroupCount, const FW::Vector3u& InThreadGroupSize, bool GlobalValidation);

		bool FinalizedThread() const { return bFinalizedThread; }
		FW::Vector3u GetWorkGroupId() const { return WorkGroupId; }
		FW::Vector3u GetLocalInvocationId() const { return LocalInvocationId; }
		void SetFinalizedThread(const FW::Vector3u& InWorkGroupId, const FW::Vector3u& InLocalInvocationId);

		// Populate the set of (workgroup -> failing local thread ids) collected by an
		// assert-highlight dispatch. Pink cells are shown for the current workgroup/z slice.
		void SetAssertedThreads(TMap<FW::Vector3u, TSet<FW::Vector3u>> InAsserted);

	private:
		void OnPickThread(const FW::Vector3u& InLocalId);
		void UpdateThreadCellStyles();

	private:
		FW::Vector3u ThreadGroupCount{ 1, 1, 1 };
		FW::Vector3u ThreadGroupSize{ 1, 1, 1 };
		FW::Vector3u WorkGroupId{ 0, 0, 0 };
		FW::Vector3u LocalInvocationId{ 0, 0, 0 };
		uint32 SelectedZ = 0;
		bool bFinalizedThread = false;

		TSharedPtr<SVerticalBox> RootBox;
		TSharedPtr<SHorizontalBox> ZSliceRow;
		TSharedPtr<class SScrollBox> HScrollBox;
		TSharedPtr<class SScrollBox> VScrollBox;
		TArray<TSharedPtr<FString>> ZSliceOptions;
		TArray<TSharedPtr<SButton>> ThreadCellButtons; // y * ThreadGroupSize.x + x
		TMap<FW::Vector3u, TSet<FW::Vector3u>> AssertedThreads;
	};
}
