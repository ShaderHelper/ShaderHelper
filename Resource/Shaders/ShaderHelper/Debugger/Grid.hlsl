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

float4 MainPS(VsOutput Input) : SV_Target
{
	float2 fragCoord = Input.Position.xy;
	float2 fragAdjusted = fragCoord + Offset;
    uint fragCol = uint(fragAdjusted.x) / Zoom;
    uint fragRow = uint(fragAdjusted.y) / Zoom;
    
    float2 mouseAdjusted = MouseLoc + Offset;
    uint mouseCol = uint(mouseAdjusted.x) / Zoom;
    uint mouseRow = uint(mouseAdjusted.y) / Zoom;

	float2 mouseCenter = float2(
        float(mouseCol) * Zoom + Zoom * 0.5,
        float(mouseRow) * Zoom + Zoom * 0.5
    ) - Offset;

	float3 fragColor = InputTex.Load(uint3(fragCol, fragRow, 0)).rgb;

    //Draw grid
    uint xd = uint(fragAdjusted.x) % Zoom;
    uint yd = uint(fragAdjusted.y) % Zoom;
    
    bool verticalEdge = xd < 1 || xd >= Zoom - 1;
    bool horizontalEdge = yd < 1 || yd >= Zoom - 1;

    float3 HighlightColor = float3(0.678, 0.847, 0.902);
    
    if (verticalEdge || horizontalEdge) {
        if (fragCol == mouseCol && fragRow == mouseRow) {
            fragColor = HighlightColor;
        } else {
            fragColor = lerp(fragColor, float3(0.5, 0.5, 0.5), Zoom / 128.0);
        }
    }

	//Draw cross
    if(!(fragCol == mouseCol && fragRow == mouseRow) && (uint(mouseCenter.x) == uint(fragCoord.x) 
    || uint(mouseCenter.y) == uint(fragCoord.y)))
    {
        fragColor = HighlightColor;
    }

	return float4(fragColor, 1.0);
}

