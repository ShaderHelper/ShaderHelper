#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Allocation.h"
#include "Dx12Util.h"

namespace FW
{	

	class Dx12Buffer : public GpuBuffer, public Dx12DeferredDeleteObject<Dx12Buffer>
	{
	public:
		Dx12Buffer(GpuBufferUsage InUsage, ResourceAllocation InAllocation, bool IsDeferred)
            : GpuBuffer(InUsage)
			, Dx12DeferredDeleteObject(IsDeferred)
			, Allocation(MoveTemp(InAllocation))
        {
			Allocation.SetOwner(this);
		}

	public:
		const ResourceAllocation& GetAllocation() const
		{
			return Allocation;
		}
        
    private:
		ResourceAllocation Allocation;
	};

	TRefCountPtr<Dx12Buffer> CreateDx12Buffer(D3D12_RESOURCE_STATES InitState, uint32 ByteSize, GpuBufferUsage Usage, bool IsDeferred = true);
	TRefCountPtr<Dx12Buffer> CreateDx12ConstantBuffer(uint32 ByteSize, GpuBufferUsage Usage, bool IsDeferred = true);
}
