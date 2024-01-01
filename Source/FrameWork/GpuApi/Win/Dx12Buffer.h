#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Allocation.h"
#include "Dx12Util.h"

namespace FRAMEWORK
{	

	class Dx12Buffer : public GpuBuffer, public Dx12DeferredDeleteObject<Dx12Buffer>
	{
	public:
		Dx12Buffer(GpuBufferUsage InUsage, ResourceAllocation InAllocation)
            : GpuBuffer(InUsage)
			, Allocation(MoveTemp(InAllocation))
        {
			Allocation.SetOwner(this);
		}

		~Dx12Buffer()
		{
			Allocation.Release();
		}

	public:
		const ResourceAllocation& GetAllocation() const
		{
			return Allocation;
		}
        
    private:
		ResourceAllocation Allocation;
	};

	TRefCountPtr<Dx12Buffer> CreateDx12Buffer(uint32 ByteSize, GpuBufferUsage Usage);
}
