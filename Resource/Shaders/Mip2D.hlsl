Texture2D InputTex : register(t0, space2);
SamplerState InputTexSampler : register(s1, space2);
RWTexture2D<float4> OutputTex : register(u2, space2);

cbuffer MipParams : register(b3, space2)
{
    uint2 OutputSize;
};

[numthreads(8, 8, 1)]
void MainCS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (any(DispatchThreadID.xy >= OutputSize)) return;
    float2 UV = (float2(DispatchThreadID.xy) + 0.5) / float2(OutputSize);
    OutputTex[DispatchThreadID.xy] = InputTex.SampleLevel(InputTexSampler, UV, 0);
}
