#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Device.h"
#include <queue>

namespace FRAMEWORK
{
	class TrackedResource;
	
	class ResourceStateTracker
	{
	public:
		void TrackResourceState(TrackedResource* InResource, D3D12_RESOURCE_STATES InState);
		void RemoveResourceState(TrackedResource* InResource);
		void SetResourceState(TrackedResource* InResource, D3D12_RESOURCE_STATES NewState);

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
		using PendingResources = TArray<TRefCountPtr<GpuResource>>;
	public:
        DeferredReleaseManager() : LastCpuFrame(0), LastGpuFrame(0)
		{
			PendingQueue.push(PendingResources{});
		}

		void AllocateOneFrame() {
			if (CurCpuFrame > LastCpuFrame)
			{
				check(CurCpuFrame == LastCpuFrame + 1);
				PendingQueue.push(PendingResources{});
				LastCpuFrame = CurCpuFrame;
			}
		}
		void AddUncompletedResource(TRefCountPtr<GpuResource> InResource) {
			PendingQueue.back().AddUnique(MoveTemp(InResource));
		}
		void ReleaseCompletedResources() {
			check(LastGpuFrame <= CurGpuFrame);
			uint64 GpuFrameIncrement = CurGpuFrame - LastGpuFrame;
			while (GpuFrameIncrement--) {
				PendingQueue.pop();
			}
			LastGpuFrame = CurGpuFrame;
		}
	private:
		uint64 LastCpuFrame, LastGpuFrame;
		std::queue<PendingResources> PendingQueue;
	};

	inline DeferredReleaseManager* GDeferredReleaseManager = new DeferredReleaseManager;

	template<typename T>
	class Dx12DeferredDeleteObject
	{
	public:
		Dx12DeferredDeleteObject()
		{
			static_assert(std::is_base_of_v<GpuResource, T>);
			GDeferredReleaseManager->AddUncompletedResource(static_cast<T*>(this));
		}
		virtual ~Dx12DeferredDeleteObject() = default;
	};
}
