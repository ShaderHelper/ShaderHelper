#include "CommonHeader.h"
#include "ShaderCodeTokenizer.h"

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

	TArray<HlslTokenizer::TokenizedLine> HlslTokenizer::Tokenize(const FString& HlslCodeString, bool IgnoreWhitespace, StateSet InLastState)
	{
		TArray<TokenizedLine> TokenizedLines;
		
		TArray<FTextRange> LineRanges;
		FTextRange::CalculateLineRangesFromString(HlslCodeString, LineRanges);

		auto StateSetToTokenType = [](const FString& TokenStr, StateSet InState) {
			switch (InState)
			{
				case StateSet::Id:
				{
					if(HLSL::BuiltinTypes.Contains(TokenStr))
					{
						return HLSL::TokenType::BuildtinType;
					}
					else if(HLSL::BuiltinFuncs.Contains(TokenStr))
					{
						return HLSL::TokenType::BuildtinFunc;
					}
					else if(HLSL::KeyWords.Contains(TokenStr))
					{
						return HLSL::TokenType::Keyword;
					}
					return HLSL::TokenType::Identifier;
				}
				case StateSet::Number:                          return HLSL::TokenType::Number;
				case StateSet::Macro:                           return HLSL::TokenType::Preprocess;
				case StateSet::Comment:                         return HLSL::TokenType::Comment;
				case StateSet::MultilineComment:                return HLSL::TokenType::Comment;
				case StateSet::MultilineCommentEnd:             return HLSL::TokenType::Comment;
				case StateSet::Punctuation:                     return HLSL::TokenType::Punctuation;
				case StateSet::NumberPuncuation:                return HLSL::TokenType::Punctuation;
				case StateSet::String:                          return HLSL::TokenType::String;
				case StateSet::StringEnd:                       return HLSL::TokenType::String;
				default:
					return HLSL::TokenType::Other;
			}
		};

		int32 CurRow = -1;
		int32 CurCol = 0;
		StateSet LastState = InLastState;
		bool bLineContinuation{};

		for (int32 Index = 0; Index < LineRanges.Num(); Index++)
		{
			const FTextRange& LineRange = LineRanges[Index];
			CurRow++;
			HlslTokenizer::TokenizedLine TokenizedLine;

			//A empty line without any char.
			if (LineRange.IsEmpty())
			{
				//Still need a token to correctly display
				TokenizedLine.State = StateSet::Start;
				TokenizedLine.Tokens.Emplace(HLSL::TokenType::Other);
				TokenizedLines.Add(MoveTemp(TokenizedLine));
			}
			else
			{
				//DFA
				StateSet CurState;
				if (Index == 0)
				{
					CurState = LastState;
				}
				else
				{
					CurState = StateSet::Start;
					if (bLineContinuation || LastState == StateSet::MultilineComment)
					{
						CurState = LastState;
						bLineContinuation = false;
					}
				}
				
				int32 CurOffset = LineRange.BeginIndex;
				int32 TokenStart = CurOffset;
				TOptional<HLSL::TokenType> LastTokenType; //The last token which is not a white space;

				bool bInMacro = false;
				bool bInComment = false;
				bool bInMultilineComment = false;
				bool bInString = false;

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
							if (!bInComment && !bInMultilineComment && CurChar == TEXT('\\') && CurOffset == LineRange.EndIndex - 1)
							{
								bLineContinuation = true;
								CurOffset += 1;
							}
							else if (!bInComment && RemainingLen >= 2 && FCString::Strncmp(CurString, TEXT("//"), 2) == 0) {
								CurState = StateSet::Comment;
							}
							else if (!bInMultilineComment && RemainingLen >= 2 && FCString::Strncmp(CurString, TEXT("/*"), 2) == 0) {
								CurState = StateSet::MultilineComment;
							}
							else if (CurChar == '"')
							{
								if (!bInString)
								{
									CurState = StateSet::String;
								}
								else
								{
									CurState = StateSet::StringEnd;
								}
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
								if(MatchedPunctuation == "*/")
								{
									CurState = StateSet::MultilineCommentEnd;
								}
								else
								{
									if (!bInComment && !bInMultilineComment && !bInString)
									{
										if (MatchedPunctuation == "{") {
											TokenizedLine.Braces.Add({ SideType::Open, CurCol });
										}
										else if (MatchedPunctuation == "}") {
											TokenizedLine.Braces.Add({ SideType::Close, CurCol });
										}
										else if (MatchedPunctuation == "(")
										{
											TokenizedLine.Parens.Add({ SideType::Open, CurCol });
										}
										else if (MatchedPunctuation == ")")
										{
											TokenizedLine.Parens.Add({ SideType::Close, CurCol });
										}
									}
									
									if (MatchedPunctuation == "+" || MatchedPunctuation == "-") {
										if (LastTokenType && (LastTokenType == HLSL::TokenType::Identifier || LastTokenType == HLSL::TokenType::Number)) {
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
						bInString = true;
						CurState = StateSet::End;
						break;
					case StateSet::StringEnd:
						LastState = StateSet::StringEnd;
						bInString = false;
						CurState = StateSet::End;
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
							if (bInMacro)
							{
								LastState = StateSet::Macro;
							}
							else if(bInComment)
							{
								LastState = StateSet::Comment;
							}
							else if(bInMultilineComment)
							{
								LastState = StateSet::MultilineComment;
							}
							else if (bInString)
							{
								LastState = StateSet::String;
							}
							HLSL::TokenType FinalTokenType = StateSetToTokenType(TokenString, LastState);

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
						bInMacro = true;
						CurState = StateSet::Start;
						break;
					case StateSet::Comment:
						LastState = StateSet::Comment;
						bInComment = true;
						CurState = StateSet::Start;
						break;
					case StateSet::MultilineComment:
						LastState = StateSet::MultilineComment;
						bInMultilineComment = true;
						CurState = StateSet::Start;
						break;
					case StateSet::MultilineCommentEnd:
						LastState = StateSet::MultilineCommentEnd;
						bInMultilineComment = false;
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
					if(LastState == StateSet::MultilineCommentEnd)
					{
						bInMultilineComment = false;
					}
				}
				if (bInMacro)
				{
					LastState = StateSet::Macro;
				}
				else if (bInComment)
				{
					LastState = StateSet::Comment;
				}
				else if (bInMultilineComment)
				{
					LastState = StateSet::MultilineComment;
				}
				else if (bInString)
				{
					LastState = StateSet::String;
				}
				FTextRange TokenRange{ TokenStart, LineRange.EndIndex };
				FString TokenString = HlslCodeString.Mid(TokenRange.BeginIndex, TokenRange.Len());
				
				if (bLineContinuation || LastState == StateSet::MultilineComment)
				{
					TokenizedLine.State = LastState;
				}
				else
				{
					TokenizedLine.State = StateSet::Start;
				}
				
				TokenizedLine.Tokens.Emplace(StateSetToTokenType(TokenString, LastState), LineRange.Len() - TokenRange.Len(), LineRange.Len());
				TokenizedLines.Add(MoveTemp(TokenizedLine));
			}
		}

		return TokenizedLines;
	}

}
