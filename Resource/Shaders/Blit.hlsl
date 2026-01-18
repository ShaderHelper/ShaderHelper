
#include "Common.hlsl"

struct VsOutput
{
	float2 UV : TEXCOORD0;
	float4 Position : SV_POSITION;
};

VsOutput MainVS(in uint VertID : SV_VertexID)
{
	VsOutput Output;
	Output.UV = float2(uint2(VertID, VertID << 1) & 2);
	Output.Position = float4(lerp(float2(-1, 1), float2(1, -1), Output.UV), 0, 1);
	return Output;
}

DECLARE_SHADER_TEXTURE(Texture2D, InputTex, 0)
DECLARE_SHADER_SAMPLER(SamplerState, InputTexSampler, 1)

float4 MainPS(VsOutput Input) : SV_Target
{
	float2 UV = Input.UV;
#if FLIP_Y
	UV.y = 1 - UV.y;	
#endif
	float4 color = InputTex.Sample(InputTexSampler, UV);
#if CHANNEL_FILTER_R
	return float4(color.r, 0, 0, 1);
#elif CHANNEL_FILTER_G
	return float4(0, color.g, 0, 1);
#elif CHANNEL_FILTER_B
	return float4(0, 0, color.b, 1);
#elif CHANNEL_FILTER_A
	return float4(0, 0, 0, color.a);
#else
	return color;
#endif
}
