#pragma once
#include "GpuResourceCommon.h"

namespace FRAMEWORK
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
