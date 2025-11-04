#pragma once
#include "GpuResourceCommon.h"

namespace FW
{
	struct GpuBufferDesc
	{
		uint32 ByteSize;
		GpuBufferUsage Usage;

		//dx structured buffer have a stride limit of 2048 bytes.
		uint32 Stride = 0;
		TArrayView<const uint8> InitialData;
	};

	class GpuBuffer : public GpuTrackedResource
	{
	public:
		GpuBuffer(GpuBufferDesc InDesc, GpuResourceState InState)
			: GpuTrackedResource(GpuResourceType::Buffer, InState)
			, Desc(MoveTemp(InDesc))
		{}

		GpuBufferUsage GetUsage() const { return Desc.Usage; }
		uint32 GetByteSize() const { return Desc.ByteSize; }

	private:
		GpuBufferDesc Desc;
	};
}
