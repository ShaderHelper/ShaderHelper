#include "Common.hlsl"

struct VsInput
{
	float3 Position : POSITION0;
	float2 UV : TEXCOORD0;
};

struct VsOutput
{
	float4 Color : COLOR0;
	float4 Position : SV_POSITION;
};

VsOutput MainVS(VsInput Input)
{
	VsOutput Output;
	Output.Position = mul(float4(Input.Position, 1.0), Transform);
	Output.Color = float4(Input.UV, 0.0, 1.0);
	return Output;
}

float4 MainPS(VsOutput Input) : SV_Target
{
	return Input.Color;
}
