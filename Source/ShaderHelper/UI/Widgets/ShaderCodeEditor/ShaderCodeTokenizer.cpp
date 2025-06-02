#include "CommonHeader.h"
#include "ShaderCodeTokenizer.h"
#include "GpuApi/GpuShader.h"

using namespace FW;

namespace SH
{	
	TOptional<int32> IsMatchPunctuation(const TCHAR* InString, int32 Len, FString& OutMatchedPuncuation)
	{
		TOptional<int32> Ret;
		for (const TCHAR* Punctuation : HLSL::Punctuations)
		{
			int32 PunctuationLength = FCString::Strlen(Punctuation);
			if (Len >= PunctuationLength && FCString::Strncmp(InString, Punctuation, PunctuationLength) == 0)
			{
				if (!Ret) {
					Ret = PunctuationLength;
				}
				else {
					Ret = FMath::Max(PunctuationLength, *Ret);
				}
				OutMatchedPuncuation = Punctuation;
			}
		}
		return Ret;
	}

	TArray<HlslHighLightTokenizer::TokenizedLine> HlslHighLightTokenizer::Tokenize(const FString& HlslCodeString, TArray<BraceGroup>& OutBraceGroups, bool IgnoreWhitespace)
	{
		TArray<HlslHighLightTokenizer::TokenizedLine> TokenizedLines;
		TArray<FTextRange> LineRanges;
		FTextRange::CalculateLineRangesFromString(HlslCodeString, LineRanges);

		enum class StateSet
		{
			Start,
			End,
			Id,
			Number,
			Macro,
			Comment,
			LeftMultilineComment,
			RightMultilineComment,
			Punctuation,
			NumberPuncuation,
			String,
			Other,
		};

		auto StateSetToTokenType = [](const FString& TokenStr, StateSet InState) {
			switch (InState)
			{
			case StateSet::Id:                              
			{
				if(HLSL::BuiltinTypes.Contains(TokenStr))
				{
					return TokenType::BuildtinType;
				}
				else if(HLSL::BuiltinFuncs.Contains(TokenStr))
				{
					return TokenType::BuildtinFunc;
				}
				else if(HLSL::KeyWords.Contains(TokenStr))
				{
					return TokenType::Keyword;
				}
				return TokenType::Identifier;
			}
			case StateSet::Number:                          return TokenType::Number;
			case StateSet::Macro:                           return TokenType::Preprocess;
			case StateSet::Comment:                         return TokenType::Comment;
			case StateSet::LeftMultilineComment:            return TokenType::Comment;
			case StateSet::RightMultilineComment:           return TokenType::Comment;
			case StateSet::Punctuation:                     return TokenType::Punctuation;
			case StateSet::NumberPuncuation:                return TokenType::Punctuation;
			case StateSet::String:                          return TokenType::String;
			case StateSet::Other:                           return TokenType::Other;
			default:
				return TokenType::Identifier;
			}
		};

		TArray<BraceGroup::BracePos> LeftBraceStack;
		int32 CurRow = -1;
		int32 CurCol = 0;

		StateSet LastLineState = StateSet::Start;

		for (const FTextRange& LineRange : LineRanges)
		{
			CurRow++;
			HlslHighLightTokenizer::TokenizedLine TokenizedLine;
			TokenizedLine.LineRange = LineRange;

			//A empty line without any char.
			if (LineRange.IsEmpty())
			{
				//Still need a token to correctly display
				TokenizedLine.Tokens.Emplace(TokenType::Other, LineRange);
				TokenizedLines.Add(MoveTemp(TokenizedLine));
			}
			else
			{
				//DFA
				StateSet CurLineState = (LastLineState == StateSet::LeftMultilineComment) ? StateSet::LeftMultilineComment : StateSet::Start;
				int32 CurOffset = LineRange.BeginIndex;
				int32 TokenStart = CurOffset;
				TOptional<TokenType> LastTokenType; //The token is not a white space;

				while (CurOffset < LineRange.EndIndex)
				{
					const TCHAR* CurString = &HlslCodeString[CurOffset];
					const TCHAR CurChar = HlslCodeString[CurOffset];
					int32 RemainingLen = HlslCodeString.Len() - CurOffset;
					CurCol = CurOffset - LineRange.BeginIndex;

					switch (CurLineState)
					{
					case StateSet::Start:
						{
							FString MatchedPunctuation;
							LastLineState = StateSet::Start;
							if (RemainingLen >= 2 && FCString::Strncmp(CurString, TEXT("//"), 2) == 0) {
								CurLineState = StateSet::Comment;
								CurOffset += 2;
							}
							else if (RemainingLen >= 2 && FCString::Strncmp(CurString, TEXT("/*"), 2) == 0) {
								CurLineState = StateSet::LeftMultilineComment;
								CurOffset += 2;
							}
							else if (CurChar == '"')
							{
								CurLineState = StateSet::String;
								CurOffset += 1;
							}
							else if (FChar::IsDigit(CurChar)) {
								CurLineState = StateSet::Number;
								CurOffset += 1;
							}
							else if (FChar::IsUnderscore(CurChar) || FChar::IsAlpha(CurChar)) {
								CurLineState = StateSet::Id;
								CurOffset += 1;
							}
							else if (CurChar == '#') {
								CurLineState = StateSet::Macro;
								CurOffset += 1;
							}
							//Skip
							else if(FChar::IsWhitespace(CurChar))
							{
								if(IgnoreWhitespace)
								{
									CurLineState = StateSet::Start;
									CurOffset += 1;
									TokenStart += 1;
								}
								else
								{
									CurLineState = StateSet::Other;
									CurOffset += 1;
								}
							}
							else if (TOptional<int32> PunctuationLen = IsMatchPunctuation(CurString, RemainingLen, MatchedPunctuation)) {

								if (MatchedPunctuation == "{") {
									LeftBraceStack.Push({ CurRow, CurCol });
								}
								else if (MatchedPunctuation == "}") {

									if (!LeftBraceStack.IsEmpty())
									{
										auto LeftBracePos = LeftBraceStack.Pop();
										BraceGroup Group{ LeftBracePos, {CurRow, CurCol}};
										OutBraceGroups.Add(MoveTemp(Group));
									}
								
								}
								
								if (MatchedPunctuation == "+" || MatchedPunctuation == "-") {
									if (LastTokenType && (LastTokenType == TokenType::Identifier || LastTokenType == TokenType::Number)) {
										CurLineState = StateSet::Punctuation;
									}
									else {
										CurLineState = StateSet::NumberPuncuation;
									}
								}
								else if (MatchedPunctuation == ".") {
									CurLineState = StateSet::NumberPuncuation;
								}
								else {
									CurLineState = StateSet::Punctuation;
								}

								CurOffset += *PunctuationLen;
							}
							else {
								CurLineState = StateSet::Other;
								CurOffset += 1;
							}
						}
						break;
					case StateSet::Other:
						LastLineState = StateSet::Other;
						//Always regard other chars as valid tokens.
						CurLineState = StateSet::End;
						break;
					case StateSet::String:
						LastLineState = StateSet::String;
						if (CurChar == '"' && HlslCodeString[CurOffset - 1] != '\\')
						{
							CurLineState = StateSet::End;
						}
						CurOffset += 1;
						break;
					case StateSet::Punctuation:
						LastLineState = StateSet::Punctuation;
						CurLineState = StateSet::End;
						break;
					case StateSet::End:
						{
							int32 TokenEnd = CurOffset;
							FTextRange TokenRange{ TokenStart, TokenEnd };
							FString TokenString = HlslCodeString.Mid(TokenRange.BeginIndex, TokenRange.Len());
							TokenType FinalTokenType = StateSetToTokenType(TokenString, LastLineState);

							if (TokenString != " ") {
								LastTokenType = FinalTokenType;
							}

							TokenizedLine.Tokens.Emplace(FinalTokenType, MoveTemp(TokenRange));
							CurLineState = StateSet::Start;
							TokenStart = TokenEnd;
						}
						break;
					case StateSet::Id:
						LastLineState = StateSet::Id;
						if (FChar::IsIdentifier(CurChar)) {
							CurLineState = StateSet::Id;
							CurOffset += 1;
						}
						else {
							CurLineState = StateSet::End;
						}
						break;
					case StateSet::NumberPuncuation:
						LastLineState = StateSet::NumberPuncuation;
						if (FChar::IsDigit(CurChar) || CurChar == '.') {
							CurLineState = StateSet::Number;
							CurOffset += 1;
						}
						else {
							CurLineState = StateSet::End;
						}
						break;
					case StateSet::Number:
						LastLineState = StateSet::Number;
						if (FChar::IsDigit(CurChar) || FChar::IsAlpha(CurChar) || CurChar == '.') {
							CurLineState = StateSet::Number;
							CurOffset += 1;
						}
						else {
							CurLineState = StateSet::End;
						}
						break;
					case StateSet::Macro:
						LastLineState = StateSet::Macro;
						if (CurOffset < LineRange.EndIndex) {
							CurLineState = StateSet::Macro;
							CurOffset += 1;
						}
						else {
							CurLineState = StateSet::End;
						}
						break;
					case StateSet::Comment:
						LastLineState = StateSet::Comment;
						if (CurOffset < LineRange.EndIndex) {
							CurLineState = StateSet::Comment;
							CurOffset += 1;
						}
						else {
							CurLineState = StateSet::End;
						}
						break;
					case StateSet::LeftMultilineComment:
						LastLineState = StateSet::LeftMultilineComment;
						if (RemainingLen >= 2 && FCString::Strncmp(CurString, TEXT("*/"), 2) == 0) {
							CurLineState = StateSet::RightMultilineComment;
							CurOffset += 2;
						}
						else {
							CurLineState = StateSet::LeftMultilineComment;
							CurOffset += 1;
						}
						break;
					case StateSet::RightMultilineComment:
						LastLineState = StateSet::RightMultilineComment;
						CurLineState = StateSet::End;
						break;
					default:
						AUX::Unreachable();
					}
				}

				//Process the last token of the line.
				if(CurLineState != StateSet::End)
				{
					LastLineState = CurLineState;
				}
				FTextRange TokenRange{ TokenStart, LineRange.EndIndex };
				FString TokenString = HlslCodeString.Mid(TokenRange.BeginIndex, TokenRange.Len());
				TokenizedLine.Tokens.Emplace(StateSetToTokenType(TokenString, LastLineState), TokenRange);

				TokenizedLines.Add(MoveTemp(TokenizedLine));
			}
		}

		return TokenizedLines;
	}

}
