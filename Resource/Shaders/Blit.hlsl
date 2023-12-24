
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

DECLARE_TEXTURE(Texture2D, InputTex, 0)
DECLARE_SAMPLER(SamplerState, InputTexSampler, 1)

float4 MainPS(VsOutput Input) : SV_Target
{
	return InputTex.Sample(InputTexSampler, Input.UV);
}