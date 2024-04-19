//fp16 texture object is not allowed under vulkan, dxc can not generate spirv
#if ENABLE_16BIT_TYPE && FINAL_HLSL
Texture2D<half> InputTex : register(t0, space0);
#else
Texture2D<float> InputTex : register(t0, space0);
#endif

void MainVS(out float4 OutPosition : SV_POSITION)
{
#if ENABLE_16BIT_TYPE && FINAL_HLSL
	OutPosition = asuint16(InputTex.Load(0));
#else
	OutPosition = InputTex.Load(0);
#endif
}
