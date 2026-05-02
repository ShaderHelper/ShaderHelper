#include "Common.hlsl"

struct VsInput
{
	float3 Position : POSITION;
	float3 Normal : NORMAL;
	float2 UV : TEXCOORD0;
};

struct VsOutput
{
	float3 WorldNormal : NORMAL;
	float2 UV : TEXCOORD0;
	float3 WorldPos : TEXCOORD1;
	float4 Position : SV_POSITION;
};

VsOutput MainVS(VsInput Input)
{
	VsOutput Output;
	float4 WorldPos = mul(float4(Input.Position, 1.0), WorldMatrix);
	Output.WorldPos = WorldPos.xyz;
	Output.Position = mul(WorldPos, ViewProjection);
	Output.WorldNormal = normalize(mul(float4(Input.Normal, 0.0), WorldMatrix).xyz);
	Output.UV = Input.UV;
	return Output;
}

float4 MainPS(VsOutput Input) : SV_Target
{
	float3 N = normalize(Input.WorldNormal);
	float3 L = normalize(-LightDir);
	
	float NdotL = saturate(dot(N, L));
	float3 Ambient = float3(0.15, 0.15, 0.18);
	float3 Diffuse = float3(0.8, 0.8, 0.8) * NdotL;
	
	// Simple rim lighting
	float3 V = normalize(CameraPos - Input.WorldPos);
	float Rim = pow(1.0 - saturate(dot(N, V)), 3.0) * 0.3;
	
	float3 FinalColor = (Ambient + Diffuse + Rim);
	return float4(FinalColor, 1.0);
}
