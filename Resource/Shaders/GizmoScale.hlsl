#include "GizmoCommon.hlsl"

// Scale gizmo shafts: 3 axes x 6 verts = 18 verts, TriangleList (screen-space quads)
static const float SHAFT_LINE_HALF_WIDTH = 1.5;

VsOutput ScaleVS(in uint VertID : SV_VertexID)
{
	float ShaftLen = GizmoScale;

	int AxisIndex = VertID / 6;
	int QuadVert = VertID % 6;

	float3 AxisDir = GetAxisDir(AxisIndex);
	float4 BaseColor = GetAxisColor(AxisIndex);

	float3 P0 = GizmoCenter;
	float3 P1 = GizmoCenter + AxisDir * ShaftLen;

	VsOutput Output;
	Output.Position = ExpandLineToQuad(P0, P1, QuadVert, SHAFT_LINE_HALF_WIDTH, BaseColor, Output.Color);
	return Output;
}

// Scale gizmo cube tips: solid cubes at axis endpoints
// Each cube: 6 faces × 2 triangles × 3 verts = 36 verts
// 3 axes = 108 verts, TriangleList
VsOutput ScaleCubeVS(in uint VertID : SV_VertexID)
{
	float ShaftLen = GizmoScale;
	float BoxHalf = GizmoScale * 0.06;

	int AxisIndex = VertID / 36;
	int LocalVert = VertID % 36;

	float3 AxisDir = GetAxisDir(AxisIndex);
	float4 BaseColor = GetAxisColor(AxisIndex);

	float3 Perp1, Perp2;
	BuildAxisBasis(AxisDir, Perp1, Perp2);

	float3 TipCenter = GizmoCenter + AxisDir * ShaftLen;

	// 8 corners of the cube
	float3 C[8];
	C[0] = TipCenter + (-Perp1 - Perp2 - AxisDir) * BoxHalf;
	C[1] = TipCenter + ( Perp1 - Perp2 - AxisDir) * BoxHalf;
	C[2] = TipCenter + ( Perp1 + Perp2 - AxisDir) * BoxHalf;
	C[3] = TipCenter + (-Perp1 + Perp2 - AxisDir) * BoxHalf;
	C[4] = TipCenter + (-Perp1 - Perp2 + AxisDir) * BoxHalf;
	C[5] = TipCenter + ( Perp1 - Perp2 + AxisDir) * BoxHalf;
	C[6] = TipCenter + ( Perp1 + Perp2 + AxisDir) * BoxHalf;
	C[7] = TipCenter + (-Perp1 + Perp2 + AxisDir) * BoxHalf;

	// 6 faces × 2 triangles = 12 triangles, 36 vertices
	// Face order: -axis, +axis, -perp2, +perp2, -perp1, +perp1
	static const int Indices[36] = {
		0,2,1, 0,3,2,  // -AxisDir face
		4,5,6, 4,6,7,  // +AxisDir face
		0,1,5, 0,5,4,  // -Perp2 face
		2,3,7, 2,7,6,  // +Perp2 face
		0,4,7, 0,7,3,  // -Perp1 face
		1,2,6, 1,6,5   // +Perp1 face
	};

	float3 WorldPos = C[Indices[LocalVert]];

	VsOutput Output;
	Output.Position = mul(float4(WorldPos, 1.0), ViewProjection);
	Output.Color = BaseColor;
	return Output;
}

// Scale center cube: single cube at GizmoCenter for uniform scale
// 36 verts, TriangleList. HighlightAxis == 4 means this cube is hovered/active.
VsOutput ScaleAllVS(in uint VertID : SV_VertexID)
{
	float BoxHalf = GizmoScale * 0.1;
	int LocalVert = VertID % 36;

	float3 AxisX = TransformAxis(float3(1, 0, 0));
	float3 AxisY = TransformAxis(float3(0, 1, 0));
	float3 AxisZ = TransformAxis(float3(0, 0, 1));

	float3 C[8];
	C[0] = GizmoCenter + (-AxisX - AxisY - AxisZ) * BoxHalf;
	C[1] = GizmoCenter + ( AxisX - AxisY - AxisZ) * BoxHalf;
	C[2] = GizmoCenter + ( AxisX + AxisY - AxisZ) * BoxHalf;
	C[3] = GizmoCenter + (-AxisX + AxisY - AxisZ) * BoxHalf;
	C[4] = GizmoCenter + (-AxisX - AxisY + AxisZ) * BoxHalf;
	C[5] = GizmoCenter + ( AxisX - AxisY + AxisZ) * BoxHalf;
	C[6] = GizmoCenter + ( AxisX + AxisY + AxisZ) * BoxHalf;
	C[7] = GizmoCenter + (-AxisX + AxisY + AxisZ) * BoxHalf;

	static const int Indices[36] = {
		0,2,1, 0,3,2,
		4,5,6, 4,6,7,
		0,1,5, 0,5,4,
		2,3,7, 2,7,6,
		0,4,7, 0,7,3,
		1,2,6, 1,6,5
	};

	float4 Color = (HighlightAxis == 4) ? float4(1.0, 1.0, 0.0, 1.0) : float4(0.85, 0.85, 0.85, 1.0);

	VsOutput Output;
	Output.Position = mul(float4(C[Indices[LocalVert]], 1.0), ViewProjection);
	Output.Color = Color;
	return Output;
}
