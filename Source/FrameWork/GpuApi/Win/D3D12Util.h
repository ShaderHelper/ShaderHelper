#pragma once
#include "D3D12Common.h"

namespace FRAMEWORK
{
	class ResourceStateTracker
	{
	public:
		void TrackResourceState(ID3D12Resource* InResource, D3D12_RESOURCE_STATES InState) {
			ResourceStateMap.Add(InResource, InState);
		}

		D3D12_RESOURCE_STATES GetResourceState(ID3D12Resource* InResource) const {
			return ResourceStateMap[InResource];
		}

		void Empty() {
			ResourceStateMap.Empty();
		}
	private:
		TMap<ID3D12Resource*, D3D12_RESOURCE_STATES> ResourceStateMap;
	};

	class ScopedBarrier
	{
	public:
		ScopedBarrier(TRefCountPtr<ID3D12Resource> InResource, D3D12_RESOURCE_STATES InDestState);
		~ScopedBarrier();
	private:
		TRefCountPtr<ID3D12Resource> Resource;
		D3D12_RESOURCE_STATES DestState;
		D3D12_RESOURCE_STATES CurState;
	};
}