#pragma once

namespace SH
{
	class HlslHighLightTokenizer
	{
	public:
		enum class TokenType
		{
			Number,
			Keyword,
			Punctuation,
			BuildtinFunc,
			BuildtinType,
			Identifier,
			Preprocess,
			Comment,
			Other,
		};

		struct Token
		{
			Token(TokenType InType, FTextRange InRange)
				: Type(InType)
				, Range(MoveTemp(InRange))
			{}

			TokenType Type;
			FTextRange Range;
		};

		struct TokenizedLine
		{
			FTextRange LineRange;
			TArray<Token> Tokens;
		};

		struct BraceGroup
		{
			struct BracePos {
				int32 Row, Col;
			};

			BracePos LeftBracePos;
			BracePos RightBracePos;
		};

	public:
		TArray<TokenizedLine> Tokenize(const FString& HlslCodeString, TArray<BraceGroup>& OutBraceGroups);
	};
}