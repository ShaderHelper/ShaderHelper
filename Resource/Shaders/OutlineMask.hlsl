#include "Common.hlsl"

struct MaskVsInput
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
	float4 Color : COLOR;
};

struct MaskVsOutput
{
	float4 Position : SV_POSITION;
};

MaskVsOutput MaskVS(MaskVsInput Input)
{
	MaskVsOutput Output;
	float4 WorldPos = mul(float4(Input.Position, 1.0), WorldMatrix);
	Output.Position = mul(WorldPos, ViewProjection);
	return Output;
}

float4 MaskPS(MaskVsOutput Input) : SV_Target
{
	return float4(1, 1, 1, 1);
}
