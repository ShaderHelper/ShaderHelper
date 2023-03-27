#include "CommonHeader.h"
#include "D3D12Util.h"
#include "D3D12CommandList.h"

namespace FRAMEWORK
{
	
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