#pragma once
#include "GpuResourceCommon.h"

namespace FRAMEWORK
{
	class GpuBuffer : public GpuResource
	{
	public:
		GpuBuffer(GpuBufferUsage InUsage) : Usage(InUsage) {}
		GpuBufferUsage GetUsage() const { return Usage; }
	private:
		GpuBufferUsage Usage;
	};
}
