#include "Common.hlsl"

struct VsOutput
{
	float2 UV : TEXCOORD0;
	float4 Position : SV_POSITION;
};

// Billboard quad: 2 triangles, 6 vertices
static const float2 kQuadOffsets[6] =
{
	float2(-0.5, -0.5), float2( 0.5, -0.5), float2( 0.5,  0.5),
	float2(-0.5, -0.5), float2( 0.5,  0.5), float2(-0.5,  0.5),
};

static const float2 kQuadUVs[6] =
{
	float2(0, 1), float2(1, 1), float2(1, 0),
	float2(0, 1), float2(1, 0), float2(0, 0),
};

VsOutput MainVS(uint VertexID : SV_VertexID)
{
	VsOutput Out;

	float2 Offset = kQuadOffsets[VertexID];
	float2 UV = kQuadUVs[VertexID];

	float3 WorldPos = BillboardPos
		+ CameraRight * (Offset.x * BillboardScale)
		+ CameraUp * (Offset.y * BillboardScale);

	Out.Position = mul(float4(WorldPos, 1.0), ViewProjection);
	Out.UV = UV;
	return Out;
}

float4 MainPS(VsOutput In) : SV_TARGET
{
	float4 Color = IconTex.Sample(IconTexSampler, In.UV);
	// Discard fully transparent pixels
	clip(Color.a - 0.01);
	return Color;
}
