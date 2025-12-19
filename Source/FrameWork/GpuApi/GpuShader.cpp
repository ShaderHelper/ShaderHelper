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
		const TCHAR* Src = *ShaderText;
		int32 SrcLen = ShaderText.Len();
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
						NewShaderText.AppendChars(Src + i, StringStart - i);
						FString PrintStringLiteral = ShaderText.Mid(StringStart, StringEnd - StringStart + 1);
						FString TextArr = TEXT("EXPAND(uint StrArr[] = {");
						int Num = PrintStringLiteral.Len() - 1;
						for (int k = 1; k < Num;)
						{
							TextArr += FString::Printf(TEXT("%d,"), PrintStringLiteral[k]);
							k++;
						}
						TextArr += TEXT("0})");
						NewShaderText += TextArr;
						i = StringEnd + 1;
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

		ShaderText = MoveTemp(NewShaderText);
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
	}

	GpuShader::GpuShader(const GpuShaderSourceDesc& SourceDesc)
		: GpuResource(GpuResourceType::Shader)
		, Type(SourceDesc.Type)
		, ShaderName(SourceDesc.Name)
		, EntryPoint(SourceDesc.EntryPoint)
		, ShaderLanguage(SourceDesc.Language)
		, SourceText(SourceDesc.Source)
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

	FString AdjustDiagLineNumber(const FString& DiagInfo, int32 Delta)
	{
		std::string DiagString{TCHAR_TO_UTF8(*DiagInfo)};
		std::regex Pattern{":([0-9]+):[0-9]+: (?:error|warning):"};
		std::smatch Match;
		std::size_t SearchPos = 0;
		while (std::regex_search(DiagString.cbegin() + SearchPos, DiagString.cend(), Match, Pattern))
		{
			std::string RowStr = Match[1];
			std::string RowNumber = std::to_string(std::stoi(RowStr) + Delta);
			DiagString.replace(SearchPos + Match.position(1), Match[1].length(), RowNumber);
			SearchPos += Match.position() + RowNumber.length();
		}
		return FString{UTF8_TO_TCHAR(DiagString.data())};
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
