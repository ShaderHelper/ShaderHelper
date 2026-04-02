#include "Common.hlsl"

struct VsInput
{
	float3 Position : POSITION0;
	float3 Normal : NORMAL0;
	float2 UV : TEXCOORD0;
};

struct VsOutput
{
	float3 Normal : NORMAL0;
	float4 Position : SV_POSITION;
};

VsOutput MainVS(VsInput Input)
{
	VsOutput Output;
	Output.Position = mul(float4(Input.Position, 1.0), Transform);
	Output.Normal = mul(float4(Input.Normal, 0.0), NormalMatrix).xyz;
	return Output;
}

float4 MainPS(VsOutput Input) : SV_Target
{
	float3 N = normalize(Input.Normal);
	float NdotL = saturate(dot(N, normalize(LightDir)));
	float3 color = float3(0.7, 0.7, 0.75) * (0.5 + 0.5 * NdotL);
	return float4(color, 1.0);
}
