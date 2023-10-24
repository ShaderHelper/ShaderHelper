#pragma once
#include "GpuResourceCommon.h"

namespace FRAMEWORK
{
    class GpuShader : public GpuResource
    {
	public:
		GpuShader() : GpuResource(GpuResourceType::Shader)
		{}

		virtual bool IsCompiled() const {
			return false;
		}
    };

	struct ShaderErrorInfo
	{
		int32 Row, Col;
		FString Info;
	};

	inline TArray<ShaderErrorInfo> ParseErrorInfoFromDxc(FStringView HlslErrorInfo)
	{
		check(HlslErrorInfo.Len());
		TArray<ShaderErrorInfo> Ret;

		int32 LineInfoFirstPos = HlslErrorInfo.Find(TEXT("hlsl.hlsl:"));
		while (LineInfoFirstPos != INDEX_NONE)
		{
			ShaderErrorInfo ErrorInfo;

			int32 LineInfoLastPos = HlslErrorInfo.Find(TEXT("\n"), LineInfoFirstPos);
			FStringView LineStringView{HlslErrorInfo.GetData() + LineInfoFirstPos, LineInfoLastPos - LineInfoFirstPos};

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
