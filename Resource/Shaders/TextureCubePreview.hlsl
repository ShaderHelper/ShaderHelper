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
	Output.SampleDir = normalize(float3(-Input.Position.z, Input.Position.y, Input.Position.x));
	return Output;
}

float4 MainPS(VsOutput Input) : SV_Target
{
	const float4 Color = PreviewCube.Sample(PreviewCubeSampler, normalize(Input.SampleDir));

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
		return float4(0.0, 0.0, 0.0, Color.a);
	}

	return Color;
}
