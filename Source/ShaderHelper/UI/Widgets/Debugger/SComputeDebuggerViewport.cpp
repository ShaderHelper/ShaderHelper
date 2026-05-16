#include "CommonHeader.h"
#include "SComputeDebuggerViewport.h"
#include "Editor/ShaderHelperEditor.h"
#include "App/App.h"

#include <Widgets/Layout/SUniformGridPanel.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Layout/SScrollBox.h>
#include <Widgets/Layout/SScrollBar.h>
#include <Widgets/Input/SNumericEntryBox.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Input/SComboBox.h>

using namespace FW;

namespace SH
{
	void SComputeDebuggerViewport::Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SAssignNew(RootBox, SVerticalBox)
		];
		RebuildBody();
	}

	void SComputeDebuggerViewport::SetComputeDebugInfo(const Vector3u& InThreadGroupCount, const Vector3u& InThreadGroupSize)
	{
		ThreadGroupCount = InThreadGroupCount;
		ThreadGroupSize = InThreadGroupSize;
		WorkGroupId = { 0, 0, 0 };
		LocalInvocationId = { 0, 0, 0 };
		SelectedZ = 0;
		bFinalizedThread = false;
		RebuildBody();
	}

	void SComputeDebuggerViewport::OnPickThread(const Vector3u& InLocalId)
	{
		auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
		if (bFinalizedThread)
		{
			if (InLocalId == LocalInvocationId)
			{
				return;
			}
			LocalInvocationId = InLocalId;
			ShEditor->SwitchDebugThread(LocalInvocationId);
			RebuildBody();
			return;
		}
		LocalInvocationId = InLocalId;
		bFinalizedThread = true;
		if (DebuggableObject* Debuggable = ShEditor->GetDebuggaleObject())
		{
			Debuggable->OnFinalizeCompute(WorkGroupId, LocalInvocationId);
		}
		RebuildBody();
	}

	FReply SComputeDebuggerViewport::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
	{
		if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
		{
			if (auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor()))
			{
				ShEditor->EndDebugging();
			}
			return FReply::Handled().ReleaseMouseCapture().ReleaseMouseLock();
		}
		return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
	}

	void SComputeDebuggerViewport::RebuildBody()
	{
		if (!RootBox.IsValid())
		{
			return;
		}
		RootBox->ClearChildren();

		const FText ThreadInfoLabel = FText::FromString(FString::Printf(TEXT("%s: (%u, %u, %u)  %s: (%u, %u, %u)"),
			*LOCALIZATION("ThreadGroupCount").ToString(), ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z,
			*LOCALIZATION("ThreadPerGroup").ToString(), ThreadGroupSize.x, ThreadGroupSize.y, ThreadGroupSize.z));

		ZSliceOptions.Reset();
		for (uint32 i = 0; i < ThreadGroupSize.z; ++i)
		{
			ZSliceOptions.Add(MakeShared<FString>(FString::Printf(TEXT("%u"), i)));
		}

		auto MakeAxisSpin = [this](int32 AxisIndex, uint32 MaxExclusive) {
			return SNew(SNumericEntryBox<int32>)
				.MinDesiredValueWidth(30.f)
				.MinValue(0)
				.MaxValue((int32)MaxExclusive - 1)
				.Value_Lambda([this, AxisIndex]() -> TOptional<int32> {
					return (int32)(AxisIndex == 0 ? WorkGroupId.x : (AxisIndex == 1 ? WorkGroupId.y : WorkGroupId.z));
				})
				.OnValueChanged_Lambda([this, AxisIndex](int32 NewValue) {
					if (AxisIndex == 0) WorkGroupId.x = (uint32)NewValue;
					else if (AxisIndex == 1) WorkGroupId.y = (uint32)NewValue;
					else WorkGroupId.z = (uint32)NewValue;
				});
		};

		// Top strip: thread info + WorkGroupId selectors.
		RootBox->AddSlot().AutoHeight().Padding(4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
			[
				SNew(STextBlock).Text(ThreadInfoLabel)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNullWidget::NullWidget
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 2, 2, 2)
			[
				SNew(STextBlock).Text(LOCALIZATION("DebugThreadGroup"))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ SNew(STextBlock).Text(FText::FromString(TEXT("X"))) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ MakeAxisSpin(0, ThreadGroupCount.x) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ SNew(STextBlock).Text(FText::FromString(TEXT("Y"))) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ MakeAxisSpin(1, ThreadGroupCount.y) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ SNew(STextBlock).Text(FText::FromString(TEXT("Z"))) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ MakeAxisSpin(2, ThreadGroupCount.z) ]
			];

		constexpr float ThreadCellSize = 16.0f;
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel)
			.SlotPadding(FMargin(2));

		static const FButtonStyle ThreadCellButtonStyle = FButtonStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));

		static const FButtonStyle SelectedThreadCellButtonStyle = [] {
			static const FSlateRoundedBoxBrush NormalBrush(FLinearColor::Green, 4.0f);
			return FButtonStyle(ThreadCellButtonStyle)
				.SetNormal(NormalBrush)
				.SetHovered(NormalBrush)
				.SetPressed(NormalBrush);
		}();

		Grid->AddSlot(0, 0)
		[
			SNew(SBox)
			.WidthOverride(ThreadCellSize)
			.HeightOverride(ThreadCellSize)
		];

		for (uint32 x = 0; x < ThreadGroupSize.x; ++x)
		{
			Grid->AddSlot((int32)x + 1, 0)
			[
				SNew(SBox)
				.WidthOverride(ThreadCellSize)
				.HeightOverride(ThreadCellSize)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%u"), x)))
					.Justification(ETextJustify::Center)
				]
			];
		}

		for (uint32 y = 0; y < ThreadGroupSize.y; ++y)
		{
			Grid->AddSlot(0, (int32)y + 1)
			[
				SNew(SBox)
				.WidthOverride(ThreadCellSize)
				.HeightOverride(ThreadCellSize)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FString::Printf(TEXT("%u"), y)))
					.Justification(ETextJustify::Center)
				]
			];

			for (uint32 x = 0; x < ThreadGroupSize.x; ++x)
			{
				Vector3u Local{ x, y, SelectedZ };
				const uint32 LocalLinear = Local.x + Local.y * ThreadGroupSize.x + Local.z * ThreadGroupSize.x * ThreadGroupSize.y;
				const bool bSelectedThread = bFinalizedThread
					&& LocalInvocationId.x == Local.x
					&& LocalInvocationId.y == Local.y
					&& LocalInvocationId.z == Local.z;
				Grid->AddSlot((int32)x + 1, (int32)y + 1)
				[
					SNew(SBox)
					.WidthOverride(ThreadCellSize)
					.HeightOverride(ThreadCellSize)
					[
						SNew(SButton)
						.ButtonStyle(bSelectedThread ? &SelectedThreadCellButtonStyle : &ThreadCellButtonStyle)
						.ContentPadding(FMargin(0))
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						.IsEnabled_Lambda([LocalLinear] {
							auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
							return ShEditor->GetDebugger().CanThreadReachStop(LocalLinear);
						})
						.OnClicked_Lambda([this, Local] {
							OnPickThread(Local);
							return FReply::Handled().ReleaseMouseLock();
						})
						.ToolTipText_Lambda([this, Local] {
							return FText::FromString(FString::Printf(TEXT("%s = (%u, %u, %u)"), *LOCALIZATION("GroupThreadID").ToString(), Local.x, Local.y, Local.z));
						})
					]
				];
			}
		}

		TSharedRef<SScrollBar> HBar = SNew(SScrollBar)
			.Orientation(Orient_Horizontal)
			.AlwaysShowScrollbar(true);
		TSharedRef<SScrollBar> VBar = SNew(SScrollBar)
			.Orientation(Orient_Vertical)
			.AlwaysShowScrollbar(true);

		RootBox->AddSlot().FillHeight(1.0f).Padding(4)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("Brushes.Recessed"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(0, 0, 0, 4)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 2, 2, 2)
					[
						SNew(STextBlock).Text(LOCALIZATION("ZSlice"))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
					[
						SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&ZSliceOptions)
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item) {
							return SNew(STextBlock).Text(FText::FromString(*Item));
						})
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewSel, ESelectInfo::Type) {
							if (NewSel.IsValid())
							{
								int32 Idx = ZSliceOptions.IndexOfByKey(NewSel);
								if (Idx != INDEX_NONE)
								{
									SelectedZ = (uint32)Idx;
									RebuildBody();
								}
							}
						})
						.InitiallySelectedItem(ZSliceOptions.IsValidIndex((int32)SelectedZ) ? ZSliceOptions[(int32)SelectedZ] : nullptr)
						[
							SNew(STextBlock).Text_Lambda([this] {
								return FText::FromString(FString::Printf(TEXT("%u"), SelectedZ));
							})
						]
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SScrollBox)
						.Orientation(EOrientation::Orient_Vertical)
						.ExternalScrollbar(VBar)
						+ SScrollBox::Slot()
						[
							SNew(SScrollBox)
							.Orientation(EOrientation::Orient_Horizontal)
							.ExternalScrollbar(HBar)						
							.ConsumeMouseWheel(EConsumeMouseWheel::Never)							
							+ SScrollBox::Slot()
							[
								Grid
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						VBar
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						HBar
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(12.0f)
					]
				]
			]
		];
	}
}
