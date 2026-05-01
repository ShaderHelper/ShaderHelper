#include "Common.hlsl"

struct VsOutput
{
	float3 WorldPos : WORLDPOS;
	float4 Position : SV_POSITION;
};

// Ground plane quad: 6 vertices, 2 triangles
VsOutput MainVS(in uint VertID : SV_VertexID)
{
	float S = GridSize;
	float3 Corners[6] = {
		float3(-S, 0, -S), float3( S, 0, -S), float3( S, 0,  S),
		float3(-S, 0, -S), float3( S, 0,  S), float3(-S, 0,  S),
	};

	VsOutput Output;
	Output.WorldPos = Corners[VertID];
	Output.Position = mul(float4(Corners[VertID], 1.0), ViewProjection);
	return Output;
}

float4 MainPS(VsOutput Input) : SV_Target
{
	// Distance-based fade
	float dist = length(Input.WorldPos.xz - CameraPos.xz);
	float fade = saturate(1.0 - dist / GridSize);

	// Grid lines
	float2 coord = Input.WorldPos.xz / GridSpacing;
	float2 dv = fwidth(coord);
	float2 grid = abs(frac(coord - 0.5) - 0.5) / dv;
	float lineDistance = min(grid.x, grid.y);
	float lineAlpha = (1.0 - saturate(lineDistance)) * fade;

	// Axis lines
	float2 absPos = abs(Input.WorldPos.xz);
	float2 dvWorld = fwidth(Input.WorldPos.xz);
	float axisX = 1.0 - saturate(absPos.y / dvWorld.y);
	float axisZ = 1.0 - saturate(absPos.x / dvWorld.x);
	if (axisZ > axisX && axisZ > 0.0)
		return float4(0.14, 0.14, 0.56, lineAlpha);
	if (axisX > 0.0)
		return float4(0.56, 0.14, 0.14, lineAlpha);

	return float4(0.5, 0.5, 0.5, lineAlpha);
}
