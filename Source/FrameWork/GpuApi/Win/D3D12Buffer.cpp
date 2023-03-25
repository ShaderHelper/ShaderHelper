#include "CommonHeader.h"
#include "D3D12Buffer.h"
#include "D3D12Device.h"
namespace FRAMEWORK
{
	
	Dx12UploadBuffer::Dx12UploadBuffer(uint64 BufferSize)
	{
		CD3DX12_HEAP_PROPERTIES HeapType{D3D12_HEAP_TYPE_UPLOAD};
		CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);
		DxCheck(GDevice->CreateCommittedResource(&HeapType, D3D12_HEAP_FLAG_NONE, 
			&BufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(Resource.GetInitReference())));
	}

}