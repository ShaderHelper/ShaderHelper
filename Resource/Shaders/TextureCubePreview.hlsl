#include "Common.hlsl"

struct VsInput
{
	float3 Position : POSITION0;
	float3 Normal : NORMAL0;
};

struct VsOutput
{
	float3 SampleDir : TEXCOORD0;
	float4 Position : SV_POSITION;
};

VsOutput MainVS(VsInput Input)
{
	VsOutput Output;
	Output.Position = mul(float4(Input.Position, 1.0), Transform);
	Output.SampleDir = Input.Position;
	return Output;
}

float4 MainPS(VsOutput Input) : SV_Target
{
	const float4 Color = PreviewCube.SampleLevel(PreviewCubeSampler, Input.SampleDir, MipLevel);

	if (ChannelFilter == 1)
	{
		return float4(Color.r, 0.0, 0.0, 1.0);
	}
	if (ChannelFilter == 2)
	{
		return float4(0.0, Color.g, 0.0, 1.0);
	}
	if (ChannelFilter == 3)
	{
		return float4(0.0, 0.0, Color.b, 1.0);
	}
	if (ChannelFilter == 4)
	{
		return float4(Color.a, Color.a, Color.a, 1.0);
	}

	return Color;
}
