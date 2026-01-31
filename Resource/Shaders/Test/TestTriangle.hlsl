#include "Common.hlsl"

struct VsOutput
{
	float4 Color : COLOR0;
	float4 Position : SV_POSITION;
};

VsOutput MainVS(in uint VertID : SV_VertexID)
{
	VsOutput Output;
	
	// Generate triangle vertices
	float2 positions[3] = {
		float2(0.0, 0.5),     // Top
		float2(-0.5, -0.5),   // Bottom left
		float2(0.5, -0.5)     // Bottom right
	};
	
	float4 colors[3] = {
		float4(1.0, 0.0, 0.0, 1.0),  // Red
		float4(0.0, 1.0, 0.0, 1.0),  // Green
		float4(0.0, 0.0, 1.0, 1.0)   // Blue
	};
	
	Output.Position = float4(positions[VertID], 0.0, 1.0);
	Output.Color = colors[VertID];
	
	return Output;
}

float4 MainPS(VsOutput Input) : SV_Target
{
	return Input.Color;
}
