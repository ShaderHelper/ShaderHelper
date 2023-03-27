#pragma once
#include "D3D12Common.h"

namespace FRAMEWORK
{
	class TrackedResource;
	
	class ResourceStateTracker
	{
	public:
		void TrackResourceState(TrackedResource* InResource, D3D12_RESOURCE_STATES InState) {
			ResourceStateMap.Add(InResource, InState);
		}

		void RemoveResourceState(TrackedResource* InResource) {
			ResourceStateMap.Remove(InResource);
		}

		D3D12_RESOURCE_STATES GetResourceState(TrackedResource* InResource) const {
			checkf(ResourceStateMap.Contains(InResource), TEXT("Querying the untracked resource."));
			return ResourceStateMap[InResource];
		}
	private:
		TMap<TrackedResource*, D3D12_RESOURCE_STATES> ResourceStateMap;
	};

	inline ResourceStateTracker GResourceStateTracker;
	
	class TrackedResource
	{
	public:
		TrackedResource(D3D12_RESOURCE_STATES InState) {
			GResourceStateTracker.TrackResourceState(this, InState);
		}
		virtual ~TrackedResource() {
			GResourceStateTracker.RemoveResourceState(this);
		}
		virtual ID3D12Resource* GetResource() const = 0;
	};

	class ScopedBarrier
	{
	public:
		ScopedBarrier(TrackedResource* InResource, D3D12_RESOURCE_STATES InDestState);
		~ScopedBarrier();
	private:
		TrackedResource* Resource;
		D3D12_RESOURCE_STATES DestState;
		D3D12_RESOURCE_STATES CurState;
	};
}