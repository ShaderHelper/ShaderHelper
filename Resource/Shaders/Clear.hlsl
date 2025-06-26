#include "Common.hlsl"

#if RESOURCE_TYPE == 1
DECLARE_SHADER_RW_BUFFER(RWStructuredBuffer<uint>, ClearResource, 0)
#endif

[numthreads(64, 1, 1)]
void ClearCS(uint3 tid : SV_DispatchThreadID)
{
	if (tid.x < Size)
	{
		ClearResource[tid.x] = 0;
	}
}