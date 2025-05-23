#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Allocation.h"
#include "Dx12Util.h"
#include "Dx12Descriptor.h"

namespace FW
{	

	class Dx12Buffer : public GpuBuffer, public Dx12DeferredDeleteObject<Dx12Buffer>
	{
	public:
		Dx12Buffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState, ResourceAllocation InAllocation, bool IsDeferred);

	public:
		const ResourceAllocation& GetAllocation() const
		{
			return Allocation;
		}

	public:
		TUniquePtr<CpuDescriptor> UAV;
        
    private:
		ResourceAllocation Allocation;
	};

	TRefCountPtr<Dx12Buffer> CreateDx12Buffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState, bool IsDeferred = true);
}
