#include "Common.hlsl"

struct VsOutput
{
	float4 Color : COLOR;
	float4 Position : SV_POSITION;
};

float3 TransformAxis(float3 Dir)
{
	return mul(float4(Dir, 0.0), GizmoOrientation).xyz;
}

float3 GetAxisDir(int AxisIndex)
{
	if (AxisIndex == 0) return TransformAxis(float3(1, 0, 0));
	if (AxisIndex == 1) return TransformAxis(float3(0, 1, 0));
	return TransformAxis(float3(0, 0, 1));
}

void BuildAxisBasis(float3 AxisDir, out float3 Perp1, out float3 Perp2)
{
	float3 RefDir = (abs(AxisDir.y) < 0.9) ? float3(0, 1, 0) : float3(1, 0, 0);
	Perp1 = normalize(cross(AxisDir, RefDir));
	Perp2 = cross(AxisDir, Perp1);
}

float4 GetAxisColor(int AxisIndex)
{
	if (HighlightAxis == AxisIndex + 1)
		return float4(1.0, 1.0, 0.0, 1.0);
	if (AxisIndex == 0) return float4(0.9, 0.2, 0.2, 1.0);
	if (AxisIndex == 1) return float4(0.2, 0.9, 0.2, 1.0);
	return float4(0.3, 0.3, 0.9, 1.0);
}

// PlaneIndex: 0=XY(HighlightAxis=5), 1=XZ(HighlightAxis=6), 2=YZ(HighlightAxis=7)
float4 GetPlaneColor(int PlaneIndex)
{
	if (HighlightAxis == PlaneIndex + 5)
		return float4(1.0, 1.0, 0.0, 0.6);
	if (PlaneIndex == 0) return float4(0.3, 0.3, 0.9, 0.4); // XY: blue
	if (PlaneIndex == 1) return float4(0.2, 0.9, 0.2, 0.4); // XZ: green
	return float4(0.9, 0.2, 0.2, 0.4); // YZ: red
}

// Expand a line segment (P0, P1) into a screen-space quad.
// QuadVert: 0-5 for the 6 vertices of 2 triangles forming a quad.
// LineHalfWidth: half-width in pixels.
// Returns the clip-space position for the given quad vertex.
float4 ExpandLineToQuad(float3 WorldP0, float3 WorldP1, int QuadVert, float LineHalfWidth, float4 BaseColor,
	out float4 OutColor)
{
	float4 Clip0 = mul(float4(WorldP0, 1.0), ViewProjection);
	float4 Clip1 = mul(float4(WorldP1, 1.0), ViewProjection);

	// NDC positions
	float2 Ndc0 = Clip0.xy / Clip0.w;
	float2 Ndc1 = Clip1.xy / Clip1.w;

	// Screen-space direction and perpendicular
	float2 ScreenDir = (Ndc1 - Ndc0) * ViewportSize * 0.5;
	float Len = length(ScreenDir);
	float2 Dir = (Len > 0.001) ? ScreenDir / Len : float2(1, 0);
	float2 Normal = float2(-Dir.y, Dir.x);

	// Offset in NDC space
	float2 OffsetNdc = Normal * LineHalfWidth * 2.0 / ViewportSize;

	// Quad vertices: 0=P0+offset, 1=P0-offset, 2=P1+offset, 3=P1-offset
	// Triangles: (0,1,2), (2,1,3)
	// QuadVert 0->corner0, 1->corner1, 2->corner2, 3->corner2, 4->corner1, 5->corner3
	int Corner;
	if      (QuadVert == 0) Corner = 0;
	else if (QuadVert == 1) Corner = 1;
	else if (QuadVert == 2) Corner = 2;
	else if (QuadVert == 3) Corner = 2;
	else if (QuadVert == 4) Corner = 1;
	else                    Corner = 3;

	float4 Clip = (Corner < 2) ? Clip0 : Clip1;
	float Side = (Corner % 2 == 0) ? 1.0 : -1.0;

	float4 Result = Clip;
	Result.xy += OffsetNdc * Clip.w * Side;

	OutColor = BaseColor;
	return Result;
}

float4 MainPS(VsOutput Input) : SV_Target
{
	return Input.Color;
}
