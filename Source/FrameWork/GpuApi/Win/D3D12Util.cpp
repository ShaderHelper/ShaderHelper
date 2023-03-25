#include "CommonHeader.h"
#include "D3D12Util.h"
#include "D3D12CommandList.h"

namespace FRAMEWORK
{
	
	ScopedBarrier::ScopedBarrier(TRefCountPtr<ID3D12Resource> InResource, D3D12_RESOURCE_STATES InDestState)
		: Resource(MoveTemp(InResource))
		, DestState(InDestState)
	{
		CurState = GCommandListContext->StateTracker.GetResourceState(Resource);
		if (CurState != DestState) {
			GCommandListContext->Transition(Resource, CurState, DestState);
		}
	}

	ScopedBarrier::~ScopedBarrier()
	{
		if (CurState != DestState) {
			GCommandListContext->Transition(Resource, DestState, CurState);
		}
	}

}