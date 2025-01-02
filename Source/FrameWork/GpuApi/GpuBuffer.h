#pragma once
#include "GpuResourceCommon.h"

namespace FW
{
	class GpuBuffer : public GpuTrackedResource
	{
	public:
		GpuBuffer(GpuBufferUsage InUsage) 
			: GpuTrackedResource(GpuResourceType::Buffer)
			, Usage(InUsage) 
		{}
		GpuBufferUsage GetUsage() const { return Usage; }

	private:
		GpuBufferUsage Usage;
	};
}
