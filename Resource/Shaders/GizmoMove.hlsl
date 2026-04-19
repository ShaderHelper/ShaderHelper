#include "GizmoCommon.hlsl"

// Move gizmo shafts: 3 axes x 6 verts = 18 verts, TriangleList (screen-space quads)
static const float SHAFT_LINE_HALF_WIDTH = 1.5;

VsOutput MoveVS(in uint VertID : SV_VertexID)
{
	float ShaftLen = GizmoScale;
	float HeadLen = GizmoScale * 0.25;

	int AxisIndex = VertID / 6;
	int QuadVert = VertID % 6;

	float3 AxisDir = GetAxisDir(AxisIndex);
	float4 BaseColor = GetAxisColor(AxisIndex);

	float EndLen = ShaftLen - HeadLen;
	float3 P0 = GizmoCenter;
	float3 P1 = GizmoCenter + AxisDir * EndLen;

	VsOutput Output;
	Output.Position = ExpandLineToQuad(P0, P1, QuadVert, SHAFT_LINE_HALF_WIDTH, BaseColor, Output.Color);
	return Output;
}

// Move gizmo cone arrowheads: solid 3D cones
// Per axis: CONE_SEGMENTS side tris + CONE_SEGMENTS base tris = CONE_SEGMENTS*6 verts
// 3 axes total
static const int CONE_SEGMENTS = 12;

VsOutput MoveArrowVS(in uint VertID : SV_VertexID)
{
	float ShaftLen = GizmoScale;
	float HeadLen = GizmoScale * 0.25;
	float HeadRadius = GizmoScale * 0.08;

	int VertsPerCone = CONE_SEGMENTS * 6;
	int AxisIndex = VertID / VertsPerCone;
	int LocalVert = VertID % VertsPerCone;

	float3 AxisDir = GetAxisDir(AxisIndex);
	float4 BaseColor = GetAxisColor(AxisIndex);

	float3 Perp1, Perp2;
	BuildAxisBasis(AxisDir, Perp1, Perp2);

	float3 TipPos = GizmoCenter + AxisDir * ShaftLen;
	float3 BaseCenter = GizmoCenter + AxisDir * (ShaftLen - HeadLen);

	int SideVerts = CONE_SEGMENTS * 3;
	float3 WorldPos;

	if (LocalVert < SideVerts)
	{
		int TriIdx = LocalVert / 3;
		int TriVert = LocalVert % 3;
		float Angle0 = TriIdx * (6.2831853 / CONE_SEGMENTS);
		float Angle1 = (TriIdx + 1) * (6.2831853 / CONE_SEGMENTS);

		if (TriVert == 0) WorldPos = TipPos;
		else if (TriVert == 1) WorldPos = BaseCenter + (cos(Angle0) * Perp1 + sin(Angle0) * Perp2) * HeadRadius;
		else WorldPos = BaseCenter + (cos(Angle1) * Perp1 + sin(Angle1) * Perp2) * HeadRadius;
	}
	else
	{
		int TriIdx = (LocalVert - SideVerts) / 3;
		int TriVert = (LocalVert - SideVerts) % 3;
		float Angle0 = TriIdx * (6.2831853 / CONE_SEGMENTS);
		float Angle1 = (TriIdx + 1) * (6.2831853 / CONE_SEGMENTS);

		if (TriVert == 0) WorldPos = BaseCenter;
		else if (TriVert == 1) WorldPos = BaseCenter + (cos(Angle1) * Perp1 + sin(Angle1) * Perp2) * HeadRadius;
		else WorldPos = BaseCenter + (cos(Angle0) * Perp1 + sin(Angle0) * Perp2) * HeadRadius;
	}

	VsOutput Output;
	Output.Position = mul(float4(WorldPos, 1.0), ViewProjection);
	Output.Color = BaseColor;
	return Output;
}
