#include "Common.hlsl"

struct VsInput
{
	float3 Position : POSITION0;
	float3 Normal : NORMAL0;
};

struct VsOutput
{
	float3 LocalPos : TEXCOORD0;
	float4 Position : SV_POSITION;
};

VsOutput MainVS(VsInput Input)
{
	VsOutput Output;
	Output.Position = mul(float4(Input.Position, 1.0), Transform);
	Output.LocalPos = Input.Position;
	return Output;
}

// Ray-AABB intersection for unit cube [-0.5, 0.5]
float2 IntersectBox(float3 rayOrigin, float3 rayDir)
{
	float3 invDir = 1.0 / rayDir;
	float3 t0 = (-0.5 - rayOrigin) * invDir;
	float3 t1 = ( 0.5 - rayOrigin) * invDir;
	float3 tMin = min(t0, t1);
	float3 tMax = max(t0, t1);
	float tNear = max(max(tMin.x, tMin.y), tMin.z);
	float tFar  = min(min(tMax.x, tMax.y), tMax.z);
	return float2(tNear, tFar);
}

float4 MainPS(VsOutput Input) : SV_Target
{
	float3 rayOrigin = CameraLocalPos;
	float3 rayDir = normalize(Input.LocalPos - CameraLocalPos);

	float2 tBounds = IntersectBox(rayOrigin, rayDir);
	tBounds.x = max(tBounds.x, 0.0);

	if (tBounds.x >= tBounds.y) discard;

	const int NumSteps = 128;
	float stepSize = (tBounds.y - tBounds.x) / NumSteps;

	float4 result = float4(0, 0, 0, 0);
	float t = tBounds.x;

	for (int i = 0; i < NumSteps; i++)
	{
		float3 pos = rayOrigin + rayDir * t;
		float3 uvw = pos + 0.5; // Map [-0.5, 0.5] to [0, 1]

		float4 s = VolumeTex.SampleLevel(VolumeSampler, uvw, MipLevel);

		// Front-to-back compositing
		float alpha = s.a * stepSize * 10.0;
		result.rgb += (1.0 - result.a) * s.rgb * alpha;
		result.a   += (1.0 - result.a) * alpha;

		if (result.a >= 0.99) break;
		t += stepSize;
	}

	if (ChannelFilter == 1)
		return float4(result.r, 0.0, 0.0, 1.0);
	if (ChannelFilter == 2)
		return float4(0.0, result.g, 0.0, 1.0);
	if (ChannelFilter == 3)
		return float4(0.0, 0.0, result.b, 1.0);
	if (ChannelFilter == 4)
		return float4(0.0, 0.0, 0.0, result.a);

	return float4(result.rgb, 1.0);
}
