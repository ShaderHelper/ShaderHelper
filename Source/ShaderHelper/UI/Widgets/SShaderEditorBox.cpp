#include "CommonHeader.h"
#include "SShaderEditorBox.h"
#include "UI/Styles/FShaderHelperStyle.h"
#include <Widgets/Text/SlateEditableTextLayout.h>
#include  <Widgets/Layout/SScrollBarTrack.h>

//No exposed methods, and too lazy to modify the source code for UE.
STEAL_PRIVATE_MEMBER(SScrollBar, TSharedPtr<SScrollBarTrack>, Track)

namespace SH
{

	const FLinearColor NormalLineNumberTextColor = { 0.3f,0.3f,0.3f,0.8f };
	const FLinearColor HighlightLineNumberTextColor = { FLinearColor::White };
	
	void SShaderEditorBox::Construct(const FArguments& InArgs)
	{
		SAssignNew(ShaderMultiLineVScrollBar, SScrollBar).Orientation(EOrientation::Orient_Vertical);
		TSharedPtr<SScrollBar> HScrollBar = SNew(SScrollBar).Orientation(EOrientation::Orient_Horizontal);

		Marshaller = MakeShared<FShaderEditorMarshaller>();

	    CurLineNum = 1;
		LineNumberData = { MakeShared<FText>(FText::FromString(FString::FromInt(CurLineNum))) };

		ChildSlot
		[
			SNew(SBorder)
			.BorderBackgroundColor(FLinearColor::Black)
			[
				SNew(SGridPanel)
				.FillColumn(0, 1.0f)
				.FillRow(0, 1.0f)
				+ SGridPanel::Slot(0, 0)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SSpacer)
						.Size(FVector2D(5, 5))
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SAssignNew(LineNumberList, SListView<LineNumberItemPtr>)
						.ListItemsSource(&LineNumberData)
						.SelectionMode(ESelectionMode::None)
						.OnGenerateRow(this, &SShaderEditorBox::GenerateRowForItem)
						.ScrollbarVisibility(EVisibility::Collapsed)
						.IsFocusable(false)
					]

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SSpacer)
						.Size(FVector2D(10, 10))
					]

					+ SHorizontalBox::Slot()
					.FillWidth(0.95f)
					[
						SAssignNew(ShaderMultiLineEditableText, SMultiLineEditableText)
						.Font(FShaderHelperStyle::Get().GetFontStyle("CodeFont"))
						.Text(this, &SShaderEditorBox::GetShadedrCode)
						.Marshaller(Marshaller)
						.OnTextChanged(this, &SShaderEditorBox::OnShaderTextChanged)
						.OnTextCommitted(this, &SShaderEditorBox::OnShadedrTextCommitted)
						.VScrollBar(ShaderMultiLineVScrollBar)
						.HScrollBar(HScrollBar)
						.OnKeyCharHandler(this, &SShaderEditorBox::OnTextKeyChar)
						.OnIsTypedCharValid_Lambda([](const TCHAR InChar) { return true; })
					]
			
				]
				+ SGridPanel::Slot(1, 0)
				[
					ShaderMultiLineVScrollBar.ToSharedRef()
				]
				+ SGridPanel::Slot(0, 1)
				[
					HScrollBar.ToSharedRef()
				]
			]
				
		];
	}

	void SShaderEditorBox::UpdateLineNumberHighlight()
	{
		const FTextSelection Selection = ShaderMultiLineEditableText->GetSelection();
		
		const int32 BeginLineNumber = Selection.GetBeginning().GetLineIndex();
		const int32 EndLineNumber = Selection.GetEnd().GetLineIndex();
		for (int32 i = 0; i < LineNumberData.Num(); i++)
		{
			LineNumberItemPtr ItemData = LineNumberData[i];
			TSharedPtr<ITableRow> ItemTableRow = LineNumberList->WidgetFromItem(ItemData);
			if (ItemTableRow.IsValid())
			{
				TSharedPtr<STextBlock> ItemWidget = StaticCastSharedPtr<STextBlock>(ItemTableRow->GetContent());
				if (i >= BeginLineNumber && i <= EndLineNumber)
				{
					ItemWidget->SetColorAndOpacity(HighlightLineNumberTextColor);
				}
				else
				{
					ItemWidget->SetColorAndOpacity(NormalLineNumberTextColor);
				}
			}
		}
	}

	void SShaderEditorBox::UpdateLineNumberListViewScrollBar()
	{
		TSharedPtr<SScrollBarTrack>& Track = GetPrivate_SScrollBar_Track(*ShaderMultiLineVScrollBar);
		float VOffsetFraction = Track->DistanceFromTop();
		LineNumberList->SetScrollOffset(VOffsetFraction * LineNumberList->GetNumItemsBeingObserved());
	}

	void SShaderEditorBox::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
	{
		SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
		
		UpdateLineNumberListViewScrollBar();
		UpdateLineNumberHighlight();
	}

	void SShaderEditorBox::OnShaderTextChanged(const FText& InText)
	{
		const int32 LineNum = Marshaller->TextLayout->GetLineCount();
		int32 DiffNum = FMath::Abs(LineNum - CurLineNum);
		if (LineNum >= CurLineNum)
		{
			while (DiffNum--)
			{
				CurLineNum++;
				LineNumberData.Add(MakeShared<FText>(FText::FromString(FString::FromInt(CurLineNum))));
			}
		}
		else
		{
			while (DiffNum--)
			{
				CurLineNum--;
				LineNumberData.Pop();
			}
		}
		ShaderCode = InText.ToString();
		LineNumberList->RequestListRefresh();
	}

	void SShaderEditorBox::OnShadedrTextCommitted(const FText& Name, ETextCommit::Type CommitInfo)
	{
		ShaderCode = Name.ToString();
	}

	static int32 GetNumSpacesAtStartOfLine(const FString& InLine)
	{
		int32 NumSpaces = 0;
		for (const TCHAR Char : InLine)
		{
			if ((Char != TEXT(' ')))
			{
				break;
			}

			NumSpaces++;
		}

		return NumSpaces;
	}

	static bool IsOpenBrace(const TCHAR& InCharacter)
	{
		return (InCharacter == TEXT('{') || InCharacter == TEXT('[') || InCharacter == TEXT('('));
	}

	static bool IsCloseBrace(const TCHAR& InCharacter)
	{
		return (InCharacter == TEXT('}') || InCharacter == TEXT(']') || InCharacter == TEXT(')'));
	}

	static TCHAR GetMatchedCloseBrace(const TCHAR& InCharacter)
	{
		return InCharacter == TEXT('{') ? TEXT('}') :
			(InCharacter == TEXT('[') ? TEXT(']') : TEXT(')'));
	}

	static bool IsWhiteSpace(const TCHAR& InCharacter)
	{
		return InCharacter == TEXT(' ') || InCharacter == TEXT('\r') || InCharacter == TEXT('\n');
	}

	void SShaderEditorBox::HandleAutoIndent() const
	{
		TSharedPtr<SMultiLineEditableText> Text = ShaderMultiLineEditableText;

		const FTextLocation CursorLocation = Text->GetCursorLocation();
		const int32 CurLineIndex = CursorLocation.GetLineIndex();
		const int32 LastLineIndex = CurLineIndex - 1;

		if (LastLineIndex > 0)
		{
			FString LastLine;
			Text->GetTextLine(LastLineIndex, LastLine);

			const int32 NumSpaces = GetNumSpacesAtStartOfLine(LastLine);

			const int32 NumSpacesForCurrentIndentation = NumSpaces / 4 * 4;
			const FString CurrentIndentation = FString::ChrN(NumSpacesForCurrentIndentation, TEXT(' '));
			const FString NextIndentation = FString::ChrN(NumSpacesForCurrentIndentation + 4, TEXT(' '));

			// See what the open/close curly brace balance is.
			int32 BraceBalance = 0;
			for (const TCHAR Char : LastLine)
			{
				BraceBalance += (Char == TEXT('{'));
				BraceBalance -= (Char == TEXT('}'));
			}

			if (BraceBalance <= 0)
			{
				Text->InsertTextAtCursor(CurrentIndentation);
			}
			else
			{
				Text->InsertTextAtCursor(NextIndentation);

				// Look for an extra close curly brace and auto-indent it as well
				FString CurLine;
				Text->GetTextLine(CurLineIndex, CurLine);

				BraceBalance = 0;
				int32 CloseBraceOffset = 0;
				for (const TCHAR Char : CurLine)
				{
					BraceBalance += (Char == TEXT('{'));
					BraceBalance -= (Char == TEXT('}'));

					// Found the first extra '}'
					if (BraceBalance < 0)
					{
						break;
					}

					CloseBraceOffset++;
				}

				if (BraceBalance < 0)
				{
					const FTextLocation SavedCursorLocation = Text->GetCursorLocation();
					const FTextLocation CloseBraceLocation(CurLineIndex, CloseBraceOffset);

					// Create a new line and apply indentation for the close curly brace
					FString NewLineAndIndent(TEXT("\n"));
					NewLineAndIndent.Append(CurrentIndentation);

					Text->GoTo(CloseBraceLocation);
					Text->InsertTextAtCursor(NewLineAndIndent);
					// Recover cursor location
					Text->GoTo(SavedCursorLocation);
				}
			}
		}
	}

	FReply SShaderEditorBox::OnTextKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent) const
	{
		TSharedPtr<SMultiLineEditableText> Text = ShaderMultiLineEditableText;
			
		if (Text->IsTextReadOnly())
		{
			return FReply::Unhandled();
		}

		const TCHAR Character = InCharacterEvent.GetCharacter();

		if (Character == TEXT('\b'))
		{
			if (!Text->AnyTextSelected())
			{
				// if we are deleting a single open brace
				// look for a matching close brace following it and delete the close brace as well
				const FTextLocation CursorLocation = Text->GetCursorLocation();
				const int Offset = CursorLocation.GetOffset();
				FString Line;
				Text->GetTextLine(CursorLocation.GetLineIndex(), Line);

				if (Line.IsValidIndex(Offset) && Line.IsValidIndex(Offset - 1))
				{
					if (IsOpenBrace(Line[Offset - 1]))
					{
						if (Line[Offset] == GetMatchedCloseBrace(Line[Offset - 1]))
						{
							SMultiLineEditableText::FScopedEditableTextTransaction ScopedTransaction(Text);
							Text->SelectText(FTextLocation(CursorLocation, -1), FTextLocation(CursorLocation, 1));
							Text->DeleteSelectedText();
							Text->GoTo(FTextLocation(CursorLocation, -1));
							return FReply::Handled();
						}
					}

				}
			}

			return FReply::Unhandled();
		}
		else if (Character == TEXT('\t'))
		{
			SMultiLineEditableText::FScopedEditableTextTransaction Transaction(Text);

			const FTextLocation CursorLocation = Text->GetCursorLocation();

			const bool bShouldIncreaseIndentation = InCharacterEvent.GetModifierKeys().IsShiftDown() ? false : true;

			// When there is no text selected, shift tab should also decrease line indentation
			const bool bShouldIndentLine = Text->AnyTextSelected() || (!Text->AnyTextSelected() && !bShouldIncreaseIndentation);

			if (bShouldIndentLine)
			{
				// Indent the whole line if there is a text selection
				const FTextSelection Selection = Text->GetSelection();
				const FTextLocation SelectionStart =
					CursorLocation == Selection.GetBeginning() ? Selection.GetEnd() : Selection.GetBeginning();

				// Shift the selection according to the new indentation
				FTextLocation NewCursorLocation;
				FTextLocation NewSelectionStart;

				const int32 StartLine = Selection.GetBeginning().GetLineIndex();
				const int32 EndLine = Selection.GetEnd().GetLineIndex();

				for (int32 Index = StartLine; Index <= EndLine; Index++)
				{
					const FTextLocation LineStart(Index, 0);
					Text->GoTo(LineStart);

					FString Line;
					Text->GetTextLine(Index, Line);
					const int32 NumSpaces = GetNumSpacesAtStartOfLine(Line);
					const int32 NumExtraSpaces = NumSpaces % 4;

					// Tab to nearest 4.
					int32 NumSpacesForIndentation;
					if (bShouldIncreaseIndentation)
					{
						NumSpacesForIndentation = NumExtraSpaces == 0 ? 4 : 4 - NumExtraSpaces;
						Text->InsertTextAtCursor(FString::ChrN(NumSpacesForIndentation, TEXT(' ')));
					}
					else
					{
						NumSpacesForIndentation = NumExtraSpaces == 0 ? FMath::Min(4, NumSpaces) : NumExtraSpaces;
						Text->SelectText(LineStart, FTextLocation(LineStart, NumSpacesForIndentation));
						Text->DeleteSelectedText();
					}

					const int32 CursorShiftDirection = bShouldIncreaseIndentation ? 1 : -1;
					const int32 CursorShift = NumSpacesForIndentation * CursorShiftDirection;

					if (Index == CursorLocation.GetLineIndex())
					{
						NewCursorLocation = FTextLocation(CursorLocation, CursorShift);
					}

					if (Index == SelectionStart.GetLineIndex())
					{
						NewSelectionStart = FTextLocation(SelectionStart, CursorShift);
					}
				}

				Text->SelectText(NewSelectionStart, NewCursorLocation);
			}
			else
			{
				FString Line;
				Text->GetCurrentTextLine(Line);

				const int32 Offset = CursorLocation.GetOffset();

				// Tab to nearest 4.
				if (ensure(bShouldIncreaseIndentation))
				{
					const int32 NumSpacesForIndentation = 4 - Offset % 4;
					Text->InsertTextAtCursor(FString::ChrN(NumSpacesForIndentation, TEXT(' ')));
				}
			}

			return FReply::Handled();
		}
		else if (Character == TEXT('\n') || Character == TEXT('\r'))
		{
			SMultiLineEditableText::FScopedEditableTextTransaction Transaction(Text);

			// at this point, the text after the text cursor is already in a new line
			HandleAutoIndent();

			return FReply::Handled();
		}
		else if (IsOpenBrace(Character))
		{
			const TCHAR CloseBrace = GetMatchedCloseBrace(Character);
			const FTextLocation CursorLocation = Text->GetCursorLocation();
			FString Line;
			Text->GetCurrentTextLine(Line);

			bool bShouldAutoInsertBraces = false;

			if (CursorLocation.GetOffset() < Line.Len())
			{
				const TCHAR NextChar = Text->GetCharacterAt(CursorLocation);

				if (IsWhiteSpace(NextChar))
				{
					bShouldAutoInsertBraces = true;
				}
				else if (IsCloseBrace(NextChar))
				{
					int32 BraceBalancePrior = 0;
					for (int32 Index = 0; Index < CursorLocation.GetOffset() && Index < Line.Len(); Index++)
					{
						BraceBalancePrior += (Line[Index] == Character);
						BraceBalancePrior -= (Line[Index] == CloseBrace);
					}

					int32 BraceBalanceLater = 0;
					for (int32 Index = CursorLocation.GetOffset(); Index < Line.Len(); Index++)
					{
						BraceBalanceLater += (Line[Index] == Character);
						BraceBalanceLater -= (Line[Index] == CloseBrace);
					}

					if (BraceBalancePrior >= -BraceBalanceLater)
					{
						bShouldAutoInsertBraces = true;
					}
				}
			}
			else
			{
				bShouldAutoInsertBraces = true;
			}

			// auto insert if we have more open braces
			// on the left side than close braces on the right side
			if (bShouldAutoInsertBraces)
			{
				// auto insert the matched close brace
				SMultiLineEditableText::FScopedEditableTextTransaction Transaction(Text);
				Text->InsertTextAtCursor(FString::Chr(Character));
				Text->InsertTextAtCursor(FString::Chr(CloseBrace));
				const FTextLocation NewCursorLocation(Text->GetCursorLocation(), -1);
				Text->GoTo(NewCursorLocation);
				return FReply::Handled();
			}

			return FReply::Unhandled();
		}
		else if (IsCloseBrace(Character))
		{
			if (!Text->AnyTextSelected())
			{
				const FTextLocation CursorLocation = Text->GetCursorLocation();
				FString Line;
				Text->GetTextLine(CursorLocation.GetLineIndex(), Line);

				const int32 Offset = CursorLocation.GetOffset();

				if (Line.IsValidIndex(Offset))
				{
					if (Line[Offset] == Character)
					{
						// avoid creating a duplicated close brace and simply
						// advance the cursor
						Text->GoTo(FTextLocation(CursorLocation, 1));
						return FReply::Handled();
					}
				}
			}

			return FReply::Unhandled();
		}
		else
		{
			// Let SMultiLineEditableText::OnKeyChar handle it.
			return FReply::Unhandled();
		}
	}

	TSharedRef<ITableRow> SShaderEditorBox::GenerateRowForItem(LineNumberItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
	{
		TSharedPtr<STextBlock> LineNumberTextBlock = SNew(STextBlock)
			.Font(FShaderHelperStyle::Get().GetFontStyle("CodeFont"))
			.ColorAndOpacity(NormalLineNumberTextColor)
			.Text(*Item)
			.Justification(ETextJustify::Right)
			.MinDesiredWidth(15.0f);
		
		return SNew(STableRow<LineNumberItemPtr>, OwnerTable)
			.Style(&FShaderHelperStyle::Get().GetWidgetStyle<FTableRowStyle>("LineNumberItemStyle"))
			.Content()
			[
				LineNumberTextBlock.ToSharedRef()
			];
	}

	FText SShaderEditorBox::GetShadedrCode() const
	{
		return FText::FromString(ShaderCode);
	}

	void FShaderEditorMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
	{
		TextLayout = &TargetTextLayout;
	}

	void FShaderEditorMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
	{
		SourceTextLayout.GetAsText(TargetString);
	}

}