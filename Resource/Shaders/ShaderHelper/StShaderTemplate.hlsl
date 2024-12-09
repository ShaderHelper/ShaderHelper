//@Binding@

void MainVS(
		in uint VertID : SV_VertexID,
		out float4 Pos : SV_Position)
{
	float2 uv = float2(uint2(VertID, VertID << 1) & 2);
	Pos = float4(lerp(float2(-1, 1), float2(1, -1), uv), 0, 1);
}

struct PIn
{
	float4 Pos : SV_Position;
};

void mainImage(out float4 fragColor, in float2 fragCoord);

float4 MainPS(PIn Input) : SV_Target
{
	float2 fragCoord = float2(Input.Pos.x, iResolution.y - Input.Pos.y);
	float4 fragColor = 1.0f;
	mainImage(fragColor, fragCoord);
	return fragColor;
}

//@mainImage@

