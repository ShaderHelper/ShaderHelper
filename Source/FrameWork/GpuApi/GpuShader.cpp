#include "CommonHeader.h"
#include "GpuShader.h"
#include "Common/Path/PathHelper.h"

namespace FW
{

	GpuShader::GpuShader(FString InFileName, ShaderType InType, const FString& ExtraDeclaration, FString InEntryPoint)
		: GpuResource(GpuResourceType::Shader)
		, Type(InType)
		, FileName(MoveTemp(InFileName))
		, EntryPoint(MoveTemp(InEntryPoint))
	{
		ShaderName = FPaths::GetBaseFilename(*FileName);
		FString ShaderFileText;
		FFileHelper::LoadFileToString(ShaderFileText, **FileName);
		SourceText = ExtraDeclaration + MoveTemp(ShaderFileText);

		IncludeDirs.Add(PathHelper::ShaderDir());
		IncludeDirs.Add(FPaths::GetPath(*FileName));
	}

	GpuShader::GpuShader(ShaderType InType, FString InSourceText, FString InShaderName, FString InEntryPoint)
		: GpuResource(GpuResourceType::Shader)
		, Type(InType)
		, ShaderName(MoveTemp(InShaderName))
		, EntryPoint(MoveTemp(InEntryPoint))
		, SourceText(MoveTemp(InSourceText))
	{

	}

	GpuShaderModel GpuShader::GetShaderModelVer() const
	{
		GpuShaderModel ShaderModel;
		if (HasFlag(GpuShaderFlag::Enable16bitType))
		{
			ShaderModel.Minor = 2;
		}
		return ShaderModel;
	}

	TArray<ShaderErrorInfo> ParseErrorInfoFromDxc(FStringView HlslErrorInfo)
	{
		check(HlslErrorInfo.Len());
		TArray<ShaderErrorInfo> Ret;

		int32 LineInfoFirstPos = HlslErrorInfo.Find(TEXT("hlsl.hlsl:"));
		while (LineInfoFirstPos != INDEX_NONE)
		{
			ShaderErrorInfo ErrorInfo;

			int32 LineInfoLastPos = HlslErrorInfo.Find(TEXT("\n"), LineInfoFirstPos);
			FStringView LineStringView{ HlslErrorInfo.GetData() + LineInfoFirstPos, LineInfoLastPos - LineInfoFirstPos };

			int32 LineInfoFirstColonPos = 9;
			int32 Pos2 = LineStringView.Find(TEXT(":"), LineInfoFirstColonPos + 1);
			ErrorInfo.Row = FCString::Atoi(LineStringView.SubStr(LineInfoFirstColonPos + 1, Pos2 - LineInfoFirstColonPos - 1).GetData());
			int32 Pos3 = LineStringView.Find(TEXT(":"), Pos2 + 1);
			ErrorInfo.Col = FCString::Atoi(LineStringView.SubStr(Pos2 + 1, Pos3 - Pos2 - 1).GetData());

			int32 ErrorPos = LineStringView.Find(TEXT("error: "), Pos3);
			if (ErrorPos != INDEX_NONE)
			{
				int32 ErrorInfoEnd = LineStringView.Find(TEXT("["), ErrorPos + 7);
				if (ErrorInfoEnd == INDEX_NONE) {
					ErrorInfoEnd = LineStringView.Len();
				}
				ErrorInfo.Info = LineStringView.SubStr(ErrorPos + 7, ErrorInfoEnd - ErrorPos - 7);
				Ret.Add(MoveTemp(ErrorInfo));
			}

			LineInfoFirstPos = HlslErrorInfo.Find(TEXT("hlsl.hlsl:"), LineInfoLastPos);
		}

		return Ret;
	}

}