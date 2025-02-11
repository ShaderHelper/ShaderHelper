//@Binding@

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
