#include "CommonHeader.h"
#include "GpuShader.h"
#include "HLSL.h"
#include "GLSL.h"

//ue regex is invalid if disable icu, so use the regex from std.
//TODO RE2
#include <regex>
#include <string>

namespace FW
{
	GpuShaderPreProcessor& GpuShaderPreProcessor::ReplacePrintStringLiteral()
	{
//		std::string ShaderString{TCHAR_TO_UTF8(*ShaderText)};
//		std::smatch Match;
//		auto ReplaceMatch = [&](const std::regex& Pattern) {
//			std::size_t SearchPos = 0;
//			while (std::regex_search(ShaderString.cbegin() + SearchPos, ShaderString.cend(), Match, Pattern))
//			{
//				FString PrintStringLiteral = UTF8_TO_TCHAR(std::string{Match[1]}.data());
//				FString TextArr = "EXPAND(uint StrArr[] = {";
//				int Num = (int)PrintStringLiteral.Len() - 1;
//				for (int i = 1; i < Num;)
//				{
//					if (i + 1 < Num && PrintStringLiteral[i] == '\\')
//					{
//						//can not use std::format, std::to_chars needs a minimum deployment target of 13.4 on mac.
//						TextArr += FString::Printf(TEXT("'\\%c',"), PrintStringLiteral[i + 1]);
//						i += 2;
//					}
//					else
//					{
//						TextArr += FString::Printf(TEXT("'%c',"), PrintStringLiteral[i]);
//						i++;
//					}
//				}
//				TextArr += "'\\0'})";
//				ShaderString.replace(SearchPos + Match.position(1), Match[1].length(), TCHAR_TO_UTF8(*TextArr));
//				SearchPos += Match.position() + TextArr.Len();
//			}
//		};
//		static std::regex PrintRegex{"Print *\\( *(\".*\")"};
//		static std::regex PrintAtMouseRegex{"PrintAtMouse *\\( *(\".*\")"};
//		static std::regex AssertRegex{"Assert *\\(.*(\".*\")"};
//		ReplaceMatch(PrintRegex);
//		ReplaceMatch(PrintAtMouseRegex);
//      ReplaceMatch(AssertRegex);
//
//		ShaderText = FString{UTF8_TO_TCHAR(ShaderString.data())};
		FString NewShaderText;
		const bool bIsGLSL = (Language == GpuShaderLanguage::GLSL);

		// Operate directly on internal ShaderText buffer.
		FString& TargetShaderText = ShaderText;

		const TCHAR* Src = *TargetShaderText;
		int32 SrcLen = TargetShaderText.Len();
		int32 i = 0;

		auto TryReplace = [&](const TCHAR* Keyword) -> bool
		{
			int32 KeywordLen = FCString::Strlen(Keyword);
			if (FCString::Strncmp(Src + i, Keyword, KeywordLen) == 0)
			{
				int32 j = i + KeywordLen;
				while (j < SrcLen && FChar::IsWhitespace(Src[j])) ++j;
				if (j < SrcLen && Src[j] == '(')
				{
					const int32 CallOpen = j;
					++j;
					int32 ParenDepth = 1;
					int32 StringStart = -1, StringEnd = -1;
					while (j < SrcLen && ParenDepth > 0)
					{
						if (Src[j] == '(') ParenDepth++;
						else if (Src[j] == ')') ParenDepth--;
						else if (Src[j] == '"' && ParenDepth == 1 && StringStart == -1)
						{
							StringStart = j;
							++j;
							while (j < SrcLen)
							{
								if (Src[j] == '"' && Src[j - 1] != '\\')
								{
									StringEnd = j;
									break;
								}
								++j;
							}
							break;
						}
						++j;
					}
					if (StringStart != -1 && StringEnd != -1)
					{
						// Find the end of the entire function call to preserve arguments after the string
						int32 CallEnd = StringEnd + 1;
						int32 Depth = 1; // already inside outer call paren
						bool bInString = false;
						while (CallEnd < SrcLen && Depth > 0)
						{
							TCHAR C = Src[CallEnd];
							if (C == '"' && (CallEnd == 0 || Src[CallEnd - 1] != '\\'))
							{
								bInString = !bInString;
							}
							else if (!bInString)
							{
								if (C == '(') Depth++;
								else if (C == ')')
								{
									Depth--;
									if (Depth == 0)
									{
										break;
									}
								}
							}
							++CallEnd;
						}

						// For GLSL we also need to adjust the function/macro name to Print0/1/2/3, Assert0/1/2/3, etc.
						if (bIsGLSL)
						{
							// Count additional arguments after the string literal within this call
							int32 AdditionalArgs = 0;
							int32 kPos = StringEnd + 1;
							int32 ArgDepth = 1; // already inside outer call paren
							bool bArgInString = false;
							while (kPos < CallEnd && ArgDepth > 0)
							{
								TCHAR C = Src[kPos];
								if (C == '"' && (kPos == 0 || Src[kPos - 1] != '\\'))
								{
									bArgInString = !bArgInString;
								}
								else if (!bArgInString)
								{
									if (C == '(') ArgDepth++;
									else if (C == ')')
									{
										ArgDepth--;
										if (ArgDepth == 0)
										{
											break;
										}
									}
									else if (C == ',' && ArgDepth == 1)
									{
										++AdditionalArgs;
									}
								}
								++kPos;
							}

							const bool bIsPrint = FCString::Strcmp(Keyword, TEXT("Print")) == 0;
							const bool bIsPrintAtMouse = FCString::Strcmp(Keyword, TEXT("PrintAtMouse")) == 0;
							const bool bIsAssert = FCString::Strcmp(Keyword, TEXT("Assert")) == 0;

							FString NewName;
							if (bIsPrint)
							{
								// Print("...", a,b,c) -> PrintN("...", a,b,c) where N = number of value args
								int32 N = FMath::Clamp(AdditionalArgs, 0, 3);
								NewName = FString::Printf(TEXT("Print%d"), N);
							}
							else if (bIsPrintAtMouse)
							{
								// PrintAtMouse("...", a,b,c) -> PrintAtMouse0 / PrintAtMouse1 / PrintAtMouse2 / PrintAtMouse3
								int32 N = FMath::Clamp(AdditionalArgs, 0, 3);
								NewName = FString::Printf(TEXT("PrintAtMouse%d"), N);
							}
							else if (bIsAssert)
							{
								// Assert(Cond, "text", v1, v2, v3) -> AssertN(Cond, "text", v1, v2, v3)
								// AdditionalArgs here is number of value args after the string
								int32 N = FMath::Clamp(AdditionalArgs, 0, 3);
								NewName = FString::Printf(TEXT("Assert%d"), N);
							}

							if (!NewName.IsEmpty())
							{
								// Up to this point NewShaderText already has Src[0..i-1]
								const int32 NameEnd = i + KeywordLen;
								NewShaderText += NewName;
								// Append everything between original name and the start of the string literal
								NewShaderText.AppendChars(Src + NameEnd, StringStart - NameEnd);
							}
							else
							{
								// Fallback: keep original text if we did not recognize the keyword
								NewShaderText.AppendChars(Src + i, StringStart - i);
							}
						}
						else
						{
							// HLSL path: keep original keyword, only replace the string literal
							NewShaderText.AppendChars(Src + i, StringStart - i);
						}

						FString PrintStringLiteral = TargetShaderText.Mid(StringStart, StringEnd - StringStart + 1);

						FString TextArr;
						if (bIsGLSL)
						{
							// GLSL: uint StrArr[] = uint[](102u, 114u, ... , 0u)
							TextArr = TEXT("EXPAND(uint StrArr[] = uint[](");
							int Num = PrintStringLiteral.Len() - 1;
							for (int k = 1; k < Num;)
							{
								TextArr += FString::Printf(TEXT("%du,"), (uint8)PrintStringLiteral[k]);
								k++;
							}
							TextArr += TEXT("0u))");
						}
						else
						{
							// HLSL: uint StrArr[] = {102,114,...,0}
							TextArr = TEXT("EXPAND(uint StrArr[] = {");
							int Num = PrintStringLiteral.Len() - 1;
							for (int k = 1; k < Num;)
							{
								TextArr += FString::Printf(TEXT("%d,"), (uint8)PrintStringLiteral[k]);
								k++;
							}
							TextArr += TEXT("0})");
						}
						NewShaderText += TextArr;
						
						// Preserve all arguments after the string literal
						if (StringEnd + 1 < CallEnd)
						{
							NewShaderText.AppendChars(Src + StringEnd + 1, CallEnd - (StringEnd + 1));
						}
						
						i = CallEnd;
						return true;
					}
					
				}
			}
			return false;
		};

		while (i < SrcLen)
		{
			if (TryReplace(TEXT("PrintAtMouse")) ||
				TryReplace(TEXT("Print")) ||
				TryReplace(TEXT("Assert")) ||
				TryReplace(TEXT("AssertFormat")) )
			{
				continue;
			}

			NewShaderText.AppendChar(Src[i]);
			++i;
		}

		TargetShaderText = MoveTemp(NewShaderText);
		return *this;
	}


	GpuShader::GpuShader(const GpuShaderFileDesc& FileDesc)
		: GpuResource(GpuResourceType::Shader)
		, Type(FileDesc.Type)
		, EntryPoint(FileDesc.EntryPoint)
		, ShaderLanguage(FileDesc.Language)
        , FileName(FileDesc.FileName)
        , IncludeDirs(FileDesc.IncludeDirs)
	{
		ShaderName = FPaths::GetBaseFilename(*FileName);
		FString ShaderFileText;
		FFileHelper::LoadFileToString(ShaderFileText, **FileName);
		SourceText = FileDesc.ExtraDecl + ShaderFileText;
		ProcessedSourceText = SourceText;
	}

	GpuShader::GpuShader(const GpuShaderSourceDesc& SourceDesc)
		: GpuResource(GpuResourceType::Shader)
		, Type(SourceDesc.Type)
		, ShaderName(SourceDesc.Name)
		, EntryPoint(SourceDesc.EntryPoint)
		, ShaderLanguage(SourceDesc.Language)
		, SourceText(SourceDesc.Source)
		, ProcessedSourceText(SourceDesc.Source)
        , IncludeDirs(SourceDesc.IncludeDirs)
	{

	}

	GpuShaderModel GpuShader::GetShaderModelVer() const
	{
		GpuShaderModel ShaderModel;
		if (EnumHasAnyFlags(CompilerFlag, GpuShaderCompilerFlag::Enable16bitType))
		{
			ShaderModel.Minor = 2;
		}
		return ShaderModel;
	}

	FRAMEWORK_API TArray<ShaderCandidateInfo> DefaultCandidates(GpuShaderLanguage Language)
	{
		TArray<ShaderCandidateInfo> Candidates;

		if (Language == GpuShaderLanguage::HLSL)
		{
			for (const FString& Type : HLSL::BuiltinTypes)
			{
				Candidates.AddUnique({ ShaderCandidateKind::Type, Type });
			}
			for (const FString& Func : HLSL::BuiltinFuncs)
			{
				Candidates.AddUnique({ ShaderCandidateKind::Func, Func });
			}
			for (const FString& Keyword : HLSL::KeyWords)
			{
				Candidates.AddUnique({ ShaderCandidateKind::Kword, Keyword });
			}
		}
		else
		{
			for (const FString& Type : GLSL::BuiltinTypes)
			{
				Candidates.AddUnique({ ShaderCandidateKind::Type, Type });
			}
			for (const FString& Func : GLSL::BuiltinFuncs)
			{
				Candidates.AddUnique({ ShaderCandidateKind::Func, Func });
			}
			for (const FString& Keyword : GLSL::KeyWords)
			{
				Candidates.AddUnique({ ShaderCandidateKind::Kword, Keyword });
			}
		}

		return Candidates;
	}

	ShaderTU::ShaderTU(const FString& InShaderSource)
        :ShaderSource(InShaderSource)
    {
		FTextRange::CalculateLineRangesFromString(ShaderSource, LineRanges);
    }

	TUniquePtr<ShaderTU> ShaderTU::Create(const FString& InShaderSource, GpuShaderLanguage Language, const TArray<FString>& IncludeDirs)
	{
		if(Language == GpuShaderLanguage::HLSL)
		{
			return MakeUnique<HlslTU>(InShaderSource, IncludeDirs);
		}
		else
		{
			return MakeUnique<GlslTU>(InShaderSource, IncludeDirs);
		}
	}

}
