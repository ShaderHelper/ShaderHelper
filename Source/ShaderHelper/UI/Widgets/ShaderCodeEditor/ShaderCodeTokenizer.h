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
		
		enum class BraceType {
			Open,
			Close,
		};
		
		struct Brace {
			BraceType Type;
			int32 Offset;
		};

		struct BraceGroup
		{
			int32 OpenLineIndex{};
			Brace OpenBrace;
			
			int32 CloseLineIndex{};
			Brace CloseBrace;
		};
		
		enum class LineContState
		{
			None,
			MultilineComment,
		};

		struct TokenizedLine
		{
			TArray<Token> Tokens;
			TArray<Brace> Braces;
			LineContState State = LineContState::None;
		};


	public:
		TArray<TokenizedLine> Tokenize(const FString& HlslCodeString, bool IgnoreWhitespace = false, LineContState InLineContState = LineContState::None);
	};
}
