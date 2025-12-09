#pragma once
#include "GpuApi/GpuShader.h"

namespace SH
{
	class HlslTokenizer
	{
	public:
		struct Token
		{
			HLSL::TokenType Type;
			//relative to line start
			int32 BeginOffset{};
			int32 EndOffset{};
		};
		
		enum class SideType {
			Open,
			Close,
		};
		
		struct Bracket {
			SideType Type;
			int32 Offset;
		};

		struct BracketGroup
		{
			int32 OpenLineIndex{};
			Bracket OpenBracket;
			
			int32 CloseLineIndex{};
			Bracket CloseBracket;
		};

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
			StringEnd,
			Whitespace,
			Other,
		};

		struct TokenizedLine
		{
			TArray<Token> Tokens;
			TArray<Bracket> Braces;
			TArray<Bracket> Parens;
			StateSet State = StateSet::Start;
		};

	public:
		TArray<TokenizedLine> Tokenize(const FString& HlslCodeString, bool IgnoreWhitespace = false, StateSet InLastState = StateSet::Start);
	};
}
