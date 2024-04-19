
Texture2D<half> InputTex : register(t0, space0);

void MainVS(out float4 OutPosition : SV_POSITION)
{
#if ENABLE_16BIT_TYPE
	OutPosition = asuint16(InputTex.Load(0));
#else
	OutPosition = InputTex.Load(0);
#endif
}