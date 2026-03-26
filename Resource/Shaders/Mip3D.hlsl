Texture3D<float4> InputTex : register(t0, space2);
SamplerState InputTexSampler : register(s1, space2);
RWTexture3D<float4> OutputTex : register(u2, space2);

cbuffer MipParams : register(b3, space2)
{
    uint3 OutputSize;
};

[numthreads(4, 4, 4)]
void MainCS(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    if (any(DispatchThreadID >= OutputSize)) return;
    float3 UV = (float3(DispatchThreadID) + 0.5) / float3(OutputSize);
    OutputTex[DispatchThreadID] = InputTex.SampleLevel(InputTexSampler, UV, 0);
}
