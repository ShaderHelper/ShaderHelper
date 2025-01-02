#pragma once
#include "Dx12Common.h"
#include "GpuApi/GpuResource.h"
#include "Dx12Device.h"
#include <queue>

namespace FW
{
	//To make sure that gpu resources do not ahead release when allow gpu lag several frames behind cpu.
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
		Dx12DeferredDeleteObject(bool IsDeferred = true)
		{
			static_assert(std::is_base_of_v<GpuResource, T>);
			if (IsDeferred) {
				GDeferredReleaseManager->AddUncompletedResource(static_cast<T*>(this));
			}
		}
		virtual ~Dx12DeferredDeleteObject() = default;
	};
}
