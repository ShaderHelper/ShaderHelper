#pragma once
#include "GpuResourceCommon.h"

namespace FW
{
	class GpuBuffer : public GpuTrackedResource
	{
	public:
		GpuBuffer(GpuBufferUsage InUsage, uint32 InByteSize) 
			: GpuTrackedResource(GpuResourceType::Buffer)
			, Usage(InUsage) 
			, ByteSize(InByteSize)
		{}

		GpuBufferUsage GetUsage() const { return Usage; }
		uint32 GetByteSize() const { return ByteSize; }

	private:
		GpuBufferUsage Usage;
		uint32 ByteSize;
	};
}
