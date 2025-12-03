#include "Common.hlsl"

#if RESOURCE_STRUCTURED_BUFFER
DECLARE_SHADER_RW_BUFFER(RWStructuredBuffer<uint>, ClearResource, 0)
#elif RESOURCE_RAW_BUFFER
DECLARE_SHADER_RW_BUFFER(RWByteAddressBuffer, ClearResource, 0)
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
