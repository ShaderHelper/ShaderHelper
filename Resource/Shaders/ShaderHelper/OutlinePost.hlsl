#include "Common.hlsl"

struct PostVsOutput
{
	float2 UV : TEXCOORD0;
	float4 Position : SV_POSITION;
};

PostVsOutput PostVS(uint VertID : SV_VertexID)
{
	PostVsOutput Output;
	Output.UV = float2(uint2(VertID, VertID << 1) & 2);
	Output.Position = float4(lerp(float2(-1, 1), float2(1, -1), Output.UV), 0, 1);
	return Output;
}

float4 PostPS(PostVsOutput Input) : SV_Target
{
	float2 texelSize = TexelSize;
	float2 uv = Input.UV;

	float tl = MaskTex.Sample(MaskTexSampler, uv + float2(-texelSize.x, texelSize.y)).r;
	float t = MaskTex.Sample(MaskTexSampler, uv + float2(0, texelSize.y)).r;
	float tr = MaskTex.Sample(MaskTexSampler, uv + float2(texelSize.x, texelSize.y)).r;
	float l = MaskTex.Sample(MaskTexSampler, uv + float2(-texelSize.x, 0)).r;
	float c = MaskTex.Sample(MaskTexSampler, uv).r;
	float r = MaskTex.Sample(MaskTexSampler, uv + float2(texelSize.x, 0)).r;
	float bl = MaskTex.Sample(MaskTexSampler, uv + float2(-texelSize.x, -texelSize.y)).r;
	float b = MaskTex.Sample(MaskTexSampler, uv + float2(0, -texelSize.y)).r;
	float br = MaskTex.Sample(MaskTexSampler, uv + float2(texelSize.x, -texelSize.y)).r;

	float sobelX = -tl - 2.0 * l - bl + tr + 2.0 * r + br;
	float sobelY = -tl - 2.0 * t - tr + bl + 2.0 * b + br;
	float edge = saturate(sqrt(sobelX * sobelX + sobelY * sobelY));
	edge *= (1.0 - c);

	return float4(OutlineColor.rgb, edge * OutlineColor.a);
}
