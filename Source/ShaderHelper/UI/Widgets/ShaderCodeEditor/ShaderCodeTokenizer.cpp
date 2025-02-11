#include "CommonHeader.h"
#include "ShaderCodeTokenizer.h"
#include "GpuApi/GpuShader.h"

using namespace FW;

namespace SH
{
	static TOptional<int32> IsMatchBultinType(const TCHAR* InString, int32 Len)
	{
		TOptional<int32> Ret;
		for (const char* Type : HLSL::BuiltinTypes)
		{
			int32 TypeLength = FCString::Strlen(ANSI_TO_TCHAR(Type));
			if (Len >= TypeLength && FCString::Strncmp(InString, ANSI_TO_TCHAR(Type), TypeLength) == 0)
			{
				if (!Ret) { 
					Ret = TypeLength; 
				}
				else {
					Ret = FMath::Max(TypeLength, *Ret);
				}
			}
		}
		return Ret;
	}
	
	TOptional<int32> IsMatchPunctuation(const TCHAR* InString, int32 Len, FString& OutMatchedPuncuation)
	{
		TOptional<int32> Ret;
		for (const char* Punctuation : HLSL::Punctuations)
		{
			int32 PunctuationLength = FCString::Strlen(ANSI_TO_TCHAR(Punctuation));
			if (Len >= PunctuationLength && FCString::Strncmp(InString, ANSI_TO_TCHAR(Punctuation), PunctuationLength) == 0)
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

	static TOptional<int32> IsMatchKeyword(const TCHAR* InString, int32 Len)
	{
		TOptional<int32> Ret;
		for (const char* Keyword : HLSL::KeyWords)
		{
			int32 KeywordLength = FCString::Strlen(ANSI_TO_TCHAR(Keyword));
			if (Len >= KeywordLength && FCString::Strncmp(InString, ANSI_TO_TCHAR(Keyword), KeywordLength) == 0)
			{
				if (!Ret) {
					Ret = KeywordLength;
				}
				else {
					Ret = FMath::Max(KeywordLength, *Ret);
				}
			}
		}
		return Ret;
	}

	static TOptional<int32> IsMatchBultinFunc(const TCHAR* InString, int32 Len)
	{
		TOptional<int32> Ret;
		for (const char* Func : HLSL::BuiltinFuncs)
		{
			int32 FuncLength = FCString::Strlen(ANSI_TO_TCHAR(Func));
			if ( Len >= FuncLength && FCString::Strncmp(InString, ANSI_TO_TCHAR(Func), FuncLength) == 0)
			{
				if (!Ret) {
					Ret = FuncLength;
				}
				else {
					Ret = FMath::Max(FuncLength, *Ret);
				}
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
			BuiltInType,
			BuiltInFunc,
			Macro,
			Comment,
			LeftMultilineComment,
			RightMultilineComment,
			Punctuation,
			NumberPuncuation,
			Keyword,
			Other,
		};

		auto StateSetToTokenType = [](StateSet InState) {
			switch (InState)
			{
			case StateSet::Id:                              return TokenType::Identifier;
			case StateSet::Number:                          return TokenType::Number;
			case StateSet::BuiltInType:                     return TokenType::BuildtinType;
			case StateSet::BuiltInFunc:                     return TokenType::BuildtinFunc;
			case StateSet::Macro:                           return TokenType::Preprocess;
			case StateSet::Comment:                         return TokenType::Comment;
			case StateSet::LeftMultilineComment:            return TokenType::Comment;
			case StateSet::RightMultilineComment:           return TokenType::Comment;
			case StateSet::Punctuation:                     return TokenType::Punctuation;
			case StateSet::NumberPuncuation:                return TokenType::Punctuation;
			case StateSet::Keyword:                         return TokenType::Keyword;
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
							else if (FChar::IsDigit(CurChar)) {
								CurLineState = StateSet::Number;
								CurOffset += 1;
							}
							else if (TOptional<int32> FuncLen = IsMatchBultinFunc(CurString, RemainingLen)) {
								CurLineState = StateSet::BuiltInFunc;
								CurOffset += *FuncLen;
							}
							else if (TOptional<int32> TypeLen = IsMatchBultinType(CurString, RemainingLen)) {
								CurLineState = StateSet::BuiltInType;
								CurOffset += *TypeLen;
							}
							else if (TOptional<int32> KeywordLen = IsMatchKeyword(CurString, RemainingLen)) {
								CurLineState = StateSet::Keyword;
								CurOffset += *KeywordLen;
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
                            else if(IgnoreWhitespace && FChar::IsWhitespace(CurChar))
                            {
                                CurLineState = StateSet::Start;
                                CurOffset += 1;
                                TokenStart += 1;
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
					case StateSet::Punctuation:
						LastLineState = StateSet::Punctuation;
						CurLineState = StateSet::End;
						break;;
					case StateSet::End:
						{
							int32 TokenEnd = CurOffset;

							TokenType FinalTokenType = StateSetToTokenType(LastLineState);
							FTextRange TokenRange{ TokenStart, TokenEnd };

							FString TokenString = HlslCodeString.Mid(TokenRange.BeginIndex, TokenRange.Len());
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
					case StateSet::BuiltInType:
						LastLineState = StateSet::BuiltInType;
						if (FChar::IsIdentifier(CurChar)) {
							CurLineState = StateSet::Id;
							CurOffset += 1;
						}
						else {
							CurLineState = StateSet::End;
						}
						break;
					case StateSet::BuiltInFunc:
						LastLineState = StateSet::BuiltInFunc;
						if (FChar::IsIdentifier(CurChar)) {
							CurLineState = StateSet::Id;
							CurOffset += 1;
						}
						else {
							CurLineState = StateSet::End;
						}
						break;
					case StateSet::Keyword:
						LastLineState = StateSet::Keyword;
						if (FChar::IsIdentifier(CurChar)) {
							CurLineState = StateSet::Id;
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
				LastLineState = CurLineState;
				TokenizedLine.Tokens.Emplace(StateSetToTokenType(LastLineState), FTextRange{ TokenStart, LineRange.EndIndex });

				TokenizedLines.Add(MoveTemp(TokenizedLine));
			}
		}

		return TokenizedLines;
	}

}
