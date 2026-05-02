#include "Common.hlsl"

struct VsOutput
{
	float4 Color : COLOR;
	float4 Position : SV_POSITION;
};

// Frustum corners in NDC (clip space before W divide):
// Near plane corners (Z = 0 in left-handed):
//   0: (-1, -1, 0)  1: ( 1, -1, 0)  2: ( 1,  1, 0)  3: (-1,  1, 0)
// Far plane corners (Z = 1):
//   4: (-1, -1, 1)  5: ( 1, -1, 1)  6: ( 1,  1, 1)  7: (-1,  1, 1)
// 12 edges: 4 near, 4 far, 4 connecting = 24 vertices (LineList)

static const float3 kNdcCorners[8] =
{
	float3(-1, -1, 0), float3( 1, -1, 0), float3( 1,  1, 0), float3(-1,  1, 0),
	float3(-1, -1, 1), float3( 1, -1, 1), float3( 1,  1, 1), float3(-1,  1, 1),
};

// 12 edges as index pairs
static const int2 kEdges[12] =
{
	// Near face
	int2(0, 1), int2(1, 2), int2(2, 3), int2(3, 0),
	// Far face
	int2(4, 5), int2(5, 6), int2(6, 7), int2(7, 4),
	// Connecting
	int2(0, 4), int2(1, 5), int2(2, 6), int2(3, 7),
};

VsOutput MainVS(uint VertexID : SV_VertexID)
{
	VsOutput Out;

	int EdgeIdx = VertexID / 2;
	int VertIdx = VertexID % 2;
	int CornerIdx = (VertIdx == 0) ? kEdges[EdgeIdx].x : kEdges[EdgeIdx].y;

	float4 NdcPos = float4(kNdcCorners[CornerIdx], 1.0);

	// Transform from NDC back to world space using inverse VP of the camera being visualized
	float4 WorldPos = mul(NdcPos, InvViewProjection);
	WorldPos /= WorldPos.w;

	// Transform to clip space using the scene camera's VP
	Out.Position = mul(WorldPos, ViewProjection);
	Out.Color = WireColor;
	return Out;
}

float4 MainPS(VsOutput In) : SV_TARGET
{
	return In.Color;
}
