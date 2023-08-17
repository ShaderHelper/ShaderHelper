#include "CommonHeader.h"
#include "Dx12Util.h"
#include "Dx12CommandList.h"

namespace FRAMEWORK
{
	void ResourceStateTracker::TrackResourceState(TrackedResource* InResource, D3D12_RESOURCE_STATES InState)
	{
		ResourceStateMap.Add(InResource, InState);
	}
	
	void ResourceStateTracker::RemoveResourceState(TrackedResource* InResource)
	{
		ResourceStateMap.Remove(InResource);
	}

	D3D12_RESOURCE_STATES ResourceStateTracker::GetResourceState(TrackedResource* InResource) const
	{
		checkf(ResourceStateMap.Contains(InResource), TEXT("Querying the untracked resource."));
		return ResourceStateMap[InResource];
	}


	ScopedBarrier::ScopedBarrier(TrackedResource* InResource, D3D12_RESOURCE_STATES InDestState)
		: Resource(InResource)
		, DestState(InDestState)
	{
		CurState = GResourceStateTracker.GetResourceState(Resource);
		if (CurState != DestState) {
			GCommandListContext->Transition(Resource->GetResource(), CurState, DestState);
		}
	}

	ScopedBarrier::~ScopedBarrier()
	{
		if (CurState != DestState) {
			GCommandListContext->Transition(Resource->GetResource(), DestState, CurState);
		}
	}

}