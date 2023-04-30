#pragma once
#include "D3D12Common.h"
#include "GpuApi/GpuResource.h"
#include "D3D12Device.h"

namespace FRAMEWORK
{
	class TrackedResource;
	
	class ResourceStateTracker
	{
	public:
		void TrackResourceState(TrackedResource* InResource, D3D12_RESOURCE_STATES InState);
		void RemoveResourceState(TrackedResource* InResource);

		D3D12_RESOURCE_STATES GetResourceState(TrackedResource* InResource) const;
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

	//To make sure that resources bound to pipeline do not ahead release when allow gpu lag several frames behind cpu.
	class DeferredReleaseManager
	{
	public:
		using PendingResourceArr = TArray<TRefCountPtr<GpuResource>>;
	public:
        DeferredReleaseManager() : LastCpuFrame(0), LastGpuFrame(0) {}
		void AllocateOneFrame() {
            PendingResources.Enqueue(PendingResourceArr{});
			LastCpuFrame = CurCpuFrame;
		}
		void AddUncompletedResource(TRefCountPtr<GpuResource> InResource) {
            PendingResourceArr* DynamicResources = PendingResources.Peek();
			DynamicResources->Add(MoveTemp(InResource));
		}
		void ReleaseCompletedResources() {
			check(LastGpuFrame <= CurGpuFrame);
			uint64 GpuFrameIncrement = CurGpuFrame - LastGpuFrame;
			while (GpuFrameIncrement--) {
                PendingResources.Pop();
			}
			LastGpuFrame = CurGpuFrame;
		}
	private:
		uint64 LastCpuFrame;
		uint64 LastGpuFrame;
		TQueue<PendingResourceArr> PendingResources;
	};

	inline DeferredReleaseManager GDeferredReleaseManager;
}
