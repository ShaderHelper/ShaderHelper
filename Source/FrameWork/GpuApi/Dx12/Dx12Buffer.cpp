#include "CommonHeader.h"
#include "Dx12Buffer.h"
#include "Dx12Map.h"
#include "Dx12GpuRhiBackend.h"

namespace FW
{
	Dx12Buffer::Dx12Buffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState, ResourceAllocation InAllocation, bool IsDeferred)
		: GpuBuffer(InBufferDesc, InResourceState)
		, Dx12DeferredDeleteObject(IsDeferred)
		, Allocation(MoveTemp(InAllocation))
	{
		Allocation.SetOwner(this);

		if (EnumHasAllFlags(InBufferDesc.Usage, GpuBufferUsage::RWStorage))
		{
			UAV = AllocCpuCbvSrvUav();
			if (Allocation.GetPolicy() == AllocationPolicy::Placed || Allocation.GetPolicy() == AllocationPolicy::Committed)
			{
				D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc{};
				UavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				UavDesc.Buffer.NumElements = InBufferDesc.ByteSize / InBufferDesc.Stride;
				UavDesc.Buffer.StructureByteStride = InBufferDesc.Stride;
				GDevice->CreateUnorderedAccessView(Allocation.GetResource(), nullptr, &UavDesc, UAV->GetHandle());
			}
		}
	}

	TRefCountPtr<Dx12Buffer> CreateDx12ConstantBuffer(const GpuBufferDesc& InBufferDesc, bool IsDeferred)
	{
		if (EnumHasAnyFlags(InBufferDesc.Usage, GpuBufferUsage::Temporary))
		{
			BumpAllocationData AllocationData = GTempUniformBufferAllocator[GetCurFrameSourceIndex()]->Alloc(InBufferDesc.ByteSize);
			return new Dx12Buffer{ InBufferDesc, GpuResourceState::UniformBuffer, AllocationData, IsDeferred };
		}
		else
		{
			BuddyAllocationData AllocationData = GPersistantUniformBufferAllocator->Alloc(InBufferDesc.ByteSize);
			return new Dx12Buffer{ InBufferDesc, GpuResourceState::UniformBuffer, AllocationData, IsDeferred };
		}
	}

	TRefCountPtr<Dx12Buffer> CreateDx12Buffer(const GpuBufferDesc& InBufferDesc, GpuResourceState InResourceState, bool IsDeferred)
	{
		if (EnumHasAllFlags(InBufferDesc.Usage, GpuBufferUsage::Uniform))
		{
			return CreateDx12ConstantBuffer(InBufferDesc, IsDeferred);
		}

		bool bHasInitialData = false;
		if (!InBufferDesc.InitialData.IsEmpty())
		{
			bHasInitialData = true;
		}

		CD3DX12_RESOURCE_DESC DxBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(InBufferDesc.ByteSize);
		GpuResourceState ResourceState = bHasInitialData ? GpuResourceState::CopyDst : InResourceState;
		D3D12_RESOURCE_STATES InitDxState = MapResourceState(ResourceState);

		TRefCountPtr<Dx12Buffer> RetBuffer;
		if (EnumHasAllFlags(InBufferDesc.Usage, GpuBufferUsage::RWStorage))
		{
			DxBufferDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(InBufferDesc.ByteSize, D3D12_HEAP_TYPE_DEFAULT, DxBufferDesc, InitDxState);
			RetBuffer = new Dx12Buffer{ InBufferDesc, ResourceState, AllocationData, IsDeferred };
		}
		else if (EnumHasAllFlags(InBufferDesc.Usage, GpuBufferUsage::Upload))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(InBufferDesc.ByteSize, D3D12_HEAP_TYPE_UPLOAD, DxBufferDesc, InitDxState);
			RetBuffer = new Dx12Buffer{ InBufferDesc, ResourceState, AllocationData, IsDeferred };
		}
		else if (EnumHasAllFlags(InBufferDesc.Usage, GpuBufferUsage::ReadBack))
		{
			CommonAllocationData AllocationData = GCommonBufferAllocator->Alloc(InBufferDesc.ByteSize, D3D12_HEAP_TYPE_READBACK, DxBufferDesc, InitDxState);
			RetBuffer = new Dx12Buffer{ InBufferDesc, ResourceState, AllocationData, IsDeferred };
		}

		checkf(RetBuffer.IsValid(), TEXT("Invalid GpuBufferUsage"));

		if (bHasInitialData)
		{
			TRefCountPtr<Dx12Buffer> UploadBuffer = CreateDx12Buffer({
				.ByteSize = InBufferDesc.ByteSize,
				.Usage = GpuBufferUsage::Upload
				},
				GpuResourceState::CopySrc
			);

			D3D12_SUBRESOURCE_DATA InitDxData = {};
			InitDxData.pData = InBufferDesc.InitialData.GetData();
			InitDxData.RowPitch = InBufferDesc.InitialData.Num();
			InitDxData.SlicePitch = InitDxData.RowPitch;

			GpuCmdRecorder* CmdRecorder = GDx12GpuRhi->BeginRecording("InitBuffer");
			{
				UpdateSubresources(static_cast<Dx12CmdRecorder*>(CmdRecorder)->GetCommandList(), RetBuffer->GetAllocation().GetResource(),
					UploadBuffer->GetAllocation().GetResource(), 0, 0, 1, &InitDxData);
				CmdRecorder->Barriers({
					{ RetBuffer, InResourceState }
				});
			}
			GDx12GpuRhi->EndRecording(CmdRecorder);
			GDx12GpuRhi->Submit({ CmdRecorder });
		}

		return RetBuffer;
	}

}
