#include "CommonHeader.h"
#include "D3D12Buffer.h"
#include "D3D12Device.h"
namespace FRAMEWORK
{
	
	Dx12Buffer::Dx12Buffer(uint64 BufferSize, BufferUsage InUsage)
	{
		CD3DX12_HEAP_PROPERTIES HeapType{ D3D12_HEAP_TYPE(InUsage) };
		D3D12_RESOURCE_STATES InitialState = InUsage == BufferUsage::Upload ? D3D12_RESOURCE_STATE_GENERIC_READ : D3D12_RESOURCE_STATE_COPY_DEST;
		CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);
		DxCheck(GDevice->CreateCommittedResource(&HeapType, D3D12_HEAP_FLAG_NONE, 
			&BufferDesc, InitialState, nullptr, IID_PPV_ARGS(Resource.GetInitReference())));
	}

}