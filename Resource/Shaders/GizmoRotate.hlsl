#include "GizmoCommon.hlsl"

// Rotate gizmo: 3 axis circles + 1 view-aligned trackball circle
// Each segment is a screen-space quad (6 verts for 2 triangles)
// Per circle: 64 segments x 6 verts = 384 verts
// 4 circles = 1536 verts, TriangleList
static const int CIRCLE_SEGMENTS = 64;
static const int VERTS_PER_CIRCLE = CIRCLE_SEGMENTS * 6;
static const float LINE_HALF_WIDTH = 1.5;

VsOutput RotateVS(in uint VertID : SV_VertexID)
{
	int CircleIndex = VertID / VERTS_PER_CIRCLE;
	int LocalVert = VertID % VERTS_PER_CIRCLE;
	int SegIndex = LocalVert / 6;
	int QuadVert = LocalVert % 6;

	float3 AxisDir;
	float4 BaseColor;
	float Radius;

	if (CircleIndex < 3)
	{
		AxisDir = GetAxisDir(CircleIndex);
		BaseColor = GetAxisColor(CircleIndex);
		Radius = GizmoScale;
	}
	else
	{
		AxisDir = normalize(GizmoCenter - CameraPos);
		Radius = GizmoScale * 1.1;
		BaseColor = (HighlightAxis == 4) ? float4(1.0, 1.0, 0.0, 1.0) : float4(1.0, 1.0, 1.0, 1.0);
	}

	float3 Perp1, Perp2;
	BuildAxisBasis(AxisDir, Perp1, Perp2);

	float Angle0 = SegIndex * (6.2831853 / CIRCLE_SEGMENTS);
	float Angle1 = (SegIndex + 1) * (6.2831853 / CIRCLE_SEGMENTS);
	float3 P0 = GizmoCenter + (cos(Angle0) * Perp1 + sin(Angle0) * Perp2) * Radius;
	float3 P1 = GizmoCenter + (cos(Angle1) * Perp1 + sin(Angle1) * Perp2) * Radius;

	// For axis circles, hide back-facing portion
	if (CircleIndex < 3)
	{
		float3 ViewDir = normalize(CameraPos - GizmoCenter);
		float MidAngle = (SegIndex + 0.5) * (6.2831853 / CIRCLE_SEGMENTS);
		float3 MidDir = cos(MidAngle) * Perp1 + sin(MidAngle) * Perp2;
		if (dot(MidDir, ViewDir) < 0)
		{
			VsOutput Output;
			Output.Position = float4(0, 0, 0, 1);
			Output.Color = float4(0, 0, 0, 0);
			return Output;
		}
	}

	VsOutput Output;
	Output.Position = ExpandLineToQuad(P0, P1, QuadVert, LINE_HALF_WIDTH, BaseColor, Output.Color);
	return Output;
}
