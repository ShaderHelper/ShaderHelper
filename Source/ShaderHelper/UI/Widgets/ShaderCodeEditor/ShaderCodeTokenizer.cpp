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

	TArray<HlslTokenizer::TokenizedLine> HlslTokenizer::Tokenize(const FString& HlslCodeString, bool IgnoreWhitespace, LineContState InLineContState)
	{
		TArray<TokenizedLine> TokenizedLines;
		
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
			MultilineComment,
			MultilineCommentEnd,
			Punctuation,
			NumberPuncuation,
			String,
			Whitespace,
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
			case StateSet::MultilineComment:                return TokenType::Comment;
			case StateSet::MultilineCommentEnd:             return TokenType::Comment;
			case StateSet::Punctuation:                     return TokenType::Punctuation;
			case StateSet::NumberPuncuation:                return TokenType::Punctuation;
			case StateSet::String:                          return TokenType::String;
			case StateSet::Other:                           return TokenType::Other;
			default:
				return TokenType::Identifier;
			}
		};

		int32 CurRow = -1;
		int32 CurCol = 0;

		StateSet LastState = InLineContState == LineContState::MultilineComment ? StateSet::MultilineComment : StateSet::Start;

		for (const FTextRange& LineRange : LineRanges)
		{
			CurRow++;
			HlslTokenizer::TokenizedLine TokenizedLine;

			//A empty line without any char.
			if (LineRange.IsEmpty())
			{
				//Still need a token to correctly display
				TokenizedLine.State = (LastState == StateSet::MultilineComment) ? LineContState::MultilineComment : LineContState::None;
				TokenizedLine.Tokens.Emplace(TokenType::Other);
				TokenizedLines.Add(MoveTemp(TokenizedLine));
			}
			else
			{
				//DFA
				StateSet CurState = (LastState == StateSet::MultilineComment) ? StateSet::MultilineComment : StateSet::Start;
				int32 CurOffset = LineRange.BeginIndex;
				int32 TokenStart = CurOffset;
				TOptional<TokenType> LastTokenType; //The token is not a white space;

				while (CurOffset < LineRange.EndIndex)
				{
					const TCHAR* CurString = &HlslCodeString[CurOffset];
					const TCHAR CurChar = HlslCodeString[CurOffset];
					int32 RemainingLen = HlslCodeString.Len() - CurOffset;
					CurCol = CurOffset - LineRange.BeginIndex;

					switch (CurState)
					{
					case StateSet::Start:
						{
							FString MatchedPunctuation;
							LastState = StateSet::Start;
							if (RemainingLen >= 2 && FCString::Strncmp(CurString, TEXT("//"), 2) == 0) {
								CurState = StateSet::Comment;
								CurOffset += 2;
							}
							else if (RemainingLen >= 2 && FCString::Strncmp(CurString, TEXT("/*"), 2) == 0) {
								CurState = StateSet::MultilineComment;
								CurOffset += 2;
							}
							else if (CurChar == '"')
							{
								CurState = StateSet::String;
								CurOffset += 1;
							}
							else if (FChar::IsDigit(CurChar)) {
								CurState = StateSet::Number;
								CurOffset += 1;
							}
							else if (FChar::IsUnderscore(CurChar) || FChar::IsAlpha(CurChar)) {
								CurState = StateSet::Id;
								CurOffset += 1;
							}
							else if (CurChar == '#') {
								CurState = StateSet::Macro;
								CurOffset += 1;
							}
							else if(FChar::IsWhitespace(CurChar))
							{
								if(IgnoreWhitespace)
								{
									CurState = StateSet::Start;
									TokenStart += 1;
								}
								else
								{
									CurState = StateSet::Whitespace;
								}
								CurOffset += 1;
							}
							else if (TOptional<int32> PunctuationLen = IsMatchPunctuation(CurString, RemainingLen, MatchedPunctuation)) {

								if (MatchedPunctuation == "{") {
									TokenizedLine.Braces.Add({BraceType::Open, CurCol });
								}
								else if (MatchedPunctuation == "}") {
									TokenizedLine.Braces.Add({BraceType::Close, CurCol });
								}
								
								if (MatchedPunctuation == "+" || MatchedPunctuation == "-") {
									if (LastTokenType && (LastTokenType == TokenType::Identifier || LastTokenType == TokenType::Number)) {
										CurState = StateSet::Punctuation;
									}
									else {
										CurState = StateSet::NumberPuncuation;
									}
								}
								else if (MatchedPunctuation == ".") {
									CurState = StateSet::NumberPuncuation;
								}
								else {
									CurState = StateSet::Punctuation;
								}

								CurOffset += *PunctuationLen;
							}
							else {
								CurState = StateSet::Other;
								CurOffset += 1;
							}
						}
						break;
					case StateSet::Whitespace:
						LastState = StateSet::Whitespace;
						if(FChar::IsWhitespace(CurChar))
						{
							CurState = StateSet::Whitespace;
							CurOffset += 1;
						}
						else
						{
							CurState = StateSet::End;
						}
						break;
					case StateSet::Other:
						LastState = StateSet::Other;
						//Always regard other chars as valid tokens.
						CurState = StateSet::End;
						break;
					case StateSet::String:
						LastState = StateSet::String;
						if (CurChar == '"' && HlslCodeString[CurOffset - 1] != '\\')
						{
							CurState = StateSet::End;
						}
						CurOffset += 1;
						break;
					case StateSet::Punctuation:
						LastState = StateSet::Punctuation;
						CurState = StateSet::End;
						break;
					case StateSet::End:
						{
							int32 TokenEnd = CurOffset;
							FTextRange TokenRange{ TokenStart, TokenEnd };
							FString TokenString = HlslCodeString.Mid(TokenRange.BeginIndex, TokenRange.Len());
							TokenType FinalTokenType = StateSetToTokenType(TokenString, LastState);

							if (TokenString != " ") {
								LastTokenType = FinalTokenType;
							}

							TokenizedLine.Tokens.Emplace(FinalTokenType, CurCol - TokenRange.Len(), CurCol);
							CurState = StateSet::Start;
							TokenStart = TokenEnd;
						}
						break;
					case StateSet::Id:
						LastState = StateSet::Id;
						if (FChar::IsIdentifier(CurChar)) {
							CurState = StateSet::Id;
							CurOffset += 1;
						}
						else {
							CurState = StateSet::End;
						}
						break;
					case StateSet::NumberPuncuation:
						LastState = StateSet::NumberPuncuation;
						if (FChar::IsDigit(CurChar) || CurChar == '.') {
							CurState = StateSet::Number;
							CurOffset += 1;
						}
						else {
							CurState = StateSet::End;
						}
						break;
					case StateSet::Number:
						LastState = StateSet::Number;
						if (FChar::IsDigit(CurChar) || FChar::IsAlpha(CurChar) || CurChar == '.') {
							CurState = StateSet::Number;
							CurOffset += 1;
						}
						else {
							CurState = StateSet::End;
						}
						break;
					case StateSet::Macro:
						LastState = StateSet::Macro;
						if (CurOffset < LineRange.EndIndex) {
							CurState = StateSet::Macro;
							CurOffset += 1;
						}
						else {
							CurState = StateSet::End;
						}
						break;
					case StateSet::Comment:
						LastState = StateSet::Comment;
						if (CurOffset < LineRange.EndIndex) {
							CurState = StateSet::Comment;
							CurOffset += 1;
						}
						else {
							CurState = StateSet::End;
						}
						break;
					case StateSet::MultilineComment:
						LastState = StateSet::MultilineComment;
						if (RemainingLen >= 2 && FCString::Strncmp(CurString, TEXT("*/"), 2) == 0) {
							CurState = StateSet::MultilineCommentEnd;
							CurOffset += 2;
						}
						else {
							CurState = StateSet::MultilineComment;
							CurOffset += 1;
						}
						break;
					case StateSet::MultilineCommentEnd:
						LastState = StateSet::MultilineCommentEnd;
						CurState = StateSet::End;
						break;
					default:
						AUX::Unreachable();
					}
				}

				//Process the last token of the line.
				if(CurState != StateSet::End)
				{
					LastState = CurState;
				}
				FTextRange TokenRange{ TokenStart, LineRange.EndIndex };
				FString TokenString = HlslCodeString.Mid(TokenRange.BeginIndex, TokenRange.Len());
				
				TokenizedLine.State = (LastState == StateSet::MultilineComment) ? LineContState::MultilineComment : LineContState::None;
				TokenizedLine.Tokens.Emplace(StateSetToTokenType(TokenString, LastState), LineRange.Len() - TokenRange.Len(), LineRange.Len());
				TokenizedLines.Add(MoveTemp(TokenizedLine));
			}
		}

		return TokenizedLines;
	}

}
