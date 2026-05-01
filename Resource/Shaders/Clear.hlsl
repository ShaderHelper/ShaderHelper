#include "Common.hlsl"

#if RESOURCE_STRUCTURED_BUFFER
RWStructuredBuffer<uint> ClearResource : register(u0, space2);
#elif RESOURCE_RAW_BUFFER
RWByteAddressBuffer ClearResource : register(u0, space2);
#endif

[numthreads(64, 1, 1)]
void ClearCS(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x < Size)
	{
#if RESOURCE_STRUCTURED_BUFFER
		ClearResource[tid.x] = 0;
#elif RESOURCE_RAW_BUFFER
		ClearResource.Store(tid.x * 4, 0);
#endif
	}
}
