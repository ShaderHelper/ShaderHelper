#include "CommonHeader.h"
#include "SComputeDebuggerViewport.h"
#include "Editor/ShaderHelperEditor.h"
#include "App/App.h"

#include <Widgets/SOverlay.h>
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
	namespace
	{
		const FButtonStyle& GetThreadCellButtonStyle()
		{
			static const FButtonStyle Style = FButtonStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
			return Style;
		}

		const FButtonStyle& GetSelectedThreadCellButtonStyle()
		{
			static const FButtonStyle Style = [] {
				static const FSlateRoundedBoxBrush NormalBrush(FLinearColor::Green, 4.0f);
				return FButtonStyle(GetThreadCellButtonStyle())
					.SetNormal(NormalBrush)
					.SetHovered(NormalBrush)
					.SetPressed(NormalBrush);
			}();
			return Style;
		}
	}

	void SComputeDebuggerViewport::Construct(const FArguments& InArgs)
	{
		ChildSlot
		[
			SAssignNew(RootBox, SVerticalBox)
		];
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
			UpdateThreadCellStyles();
			return;
		}
		LocalInvocationId = InLocalId;
		bFinalizedThread = true;
		if (DebuggableObject* Debuggable = ShEditor->GetDebuggaleObject())
		{
			Debuggable->OnFinalizeCompute(WorkGroupId, LocalInvocationId);
		}
		UpdateThreadCellStyles();
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

	void SComputeDebuggerViewport::SetComputeDebugInfo(const Vector3u& InThreadGroupCount, const Vector3u& InThreadGroupSize)
	{
		ThreadGroupCount = InThreadGroupCount;
		ThreadGroupSize = InThreadGroupSize;
		WorkGroupId = { 0, 0, 0 };
		LocalInvocationId = { 0, 0, 0 };
		SelectedZ = 0;
		bFinalizedThread = false;

		RootBox->ClearChildren();
		ThreadCellButtons.Reset();

		ZSliceOptions.Reset();
		for (uint32 i = 0; i < ThreadGroupSize.z; ++i)
		{
			ZSliceOptions.Add(MakeShared<FString>(FString::Printf(TEXT("%u"), i)));
		}

		auto MakeAxisSpin = [this](int32 AxisIndex) {
			return SNew(SNumericEntryBox<int32>)
				.MinDesiredValueWidth(30.f)
				.MinValue(0)
				.MaxValue_Lambda([this, AxisIndex]() -> TOptional<int32> {
					const uint32 Max = (AxisIndex == 0 ? ThreadGroupCount.x : (AxisIndex == 1 ? ThreadGroupCount.y : ThreadGroupCount.z));
					return (int32)Max - 1;
				})
				.Value_Lambda([this, AxisIndex]() -> TOptional<int32> {
					return (int32)(AxisIndex == 0 ? WorkGroupId.x : (AxisIndex == 1 ? WorkGroupId.y : WorkGroupId.z));
				})
				.OnValueChanged_Lambda([this, AxisIndex](int32 NewValue) {
					if (AxisIndex == 0) WorkGroupId.x = (uint32)NewValue;
					else if (AxisIndex == 1) WorkGroupId.y = (uint32)NewValue;
					else WorkGroupId.z = (uint32)NewValue;
				});
		};

		RootBox->AddSlot().AutoHeight().Padding(4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
			[
				SNew(STextBlock).Text_Lambda([this] {
					return FText::FromString(FString::Printf(TEXT("%s: (%u, %u, %u)  %s: (%u, %u, %u)"),
						*LOCALIZATION("ThreadGroupCount").ToString(), ThreadGroupCount.x, ThreadGroupCount.y, ThreadGroupCount.z,
						*LOCALIZATION("ThreadPerGroup").ToString(), ThreadGroupSize.x, ThreadGroupSize.y, ThreadGroupSize.z));
				})
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
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ MakeAxisSpin(0) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ SNew(STextBlock).Text(FText::FromString(TEXT("Y"))) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ MakeAxisSpin(1) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ SNew(STextBlock).Text(FText::FromString(TEXT("Z"))) ]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ MakeAxisSpin(2) ]
			];

		constexpr float ThreadCellSize = 16.0f;
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel)
			.SlotPadding(FMargin(2));

		ThreadCellButtons.SetNum(ThreadGroupSize.x * ThreadGroupSize.y);
		const FText DisabledThreadCellText = FText::FromString(TEXT("x"));

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
				TSharedPtr<SButton> Btn;
				Grid->AddSlot((int32)x + 1, (int32)y + 1)
				[
					SNew(SBox)
					.WidthOverride(ThreadCellSize)
					.HeightOverride(ThreadCellSize)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SAssignNew(Btn, SButton)
							.ButtonStyle(&GetThreadCellButtonStyle())
							.ContentPadding(FMargin(0))
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							.IsEnabled_Lambda([this, x, y] {
								const uint32 LocalLinear = x + y * ThreadGroupSize.x + SelectedZ * ThreadGroupSize.x * ThreadGroupSize.y;
								auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
								return ShEditor->GetDebugger().CanThreadReachStop(LocalLinear);
							})
							.OnClicked_Lambda([this, x, y] {
								OnPickThread(Vector3u{ x, y, SelectedZ });
								return FReply::Handled().ReleaseMouseLock();
							})
							.ToolTipText_Lambda([this, x, y] {
								return FText::FromString(FString::Printf(TEXT("%s = (%u, %u, %u)"), *LOCALIZATION("GroupThreadID").ToString(), x, y, SelectedZ));
							})
						]
						+ SOverlay::Slot()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(DisabledThreadCellText)
							.Visibility_Lambda([this, x, y] {
								const uint32 LocalLinear = x + y * ThreadGroupSize.x + SelectedZ * ThreadGroupSize.x * ThreadGroupSize.y;
								auto* ShEditor = static_cast<ShaderHelperEditor*>(GApp->GetEditor());
								return ShEditor->GetDebugger().CanThreadReachStop(LocalLinear) ? EVisibility::Collapsed : EVisibility::HitTestInvisible;
							})
							.Justification(ETextJustify::Center)
						]
					]
				];
				ThreadCellButtons[y * ThreadGroupSize.x + x] = Btn;
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
									UpdateThreadCellStyles();
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

		UpdateThreadCellStyles();
	}

	void SComputeDebuggerViewport::UpdateThreadCellStyles()
	{
		for (uint32 y = 0; y < ThreadGroupSize.y; ++y)
		{
			for (uint32 x = 0; x < ThreadGroupSize.x; ++x)
			{
				const TSharedPtr<SButton>& Btn = ThreadCellButtons[y * ThreadGroupSize.x + x];
				if (!Btn.IsValid())
				{
					continue;
				}
				const bool bSelected = bFinalizedThread
					&& LocalInvocationId.x == x
					&& LocalInvocationId.y == y
					&& LocalInvocationId.z == SelectedZ;
				Btn->SetButtonStyle(bSelected ? &GetSelectedThreadCellButtonStyle() : &GetThreadCellButtonStyle());
			}
		}
	}
}
