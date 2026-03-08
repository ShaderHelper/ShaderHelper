#pragma once
#include "GpuResourceCommon.h"

namespace FW
{
	struct GpuBufferDesc
	{
		uint32 ByteSize;
		GpuBufferUsage Usage;
		TArrayView<const uint8> InitialData;

		union
		{
			struct
			{
				//dx structured buffer have a stride limit of 2048 bytes.
				uint32 Stride = 0;
			} StructuredInit;
		};
	};

	class GpuBuffer : public GpuResource
	{
	public:
		GpuBuffer(GpuBufferDesc InDesc, GpuResourceState InState)
			: GpuResource(GpuResourceType::Buffer)
			, Desc(MoveTemp(InDesc))
			, State(InState)
		{}

		GpuBufferUsage GetUsage() const { return Desc.Usage; }
		uint32 GetByteSize() const { return Desc.ByteSize; }

		GpuResourceState State;

	private:
		GpuBufferDesc Desc;
	};
}
