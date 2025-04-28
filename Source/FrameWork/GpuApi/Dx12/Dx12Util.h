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
		struct PendingResource
		{
			TRefCountPtr<GpuResource> Resource;
			uint64 WaitFrame;
		};
	public:
		void AddResource(TRefCountPtr<GpuResource> InResource) 
		{
			Resources.Add(MoveTemp(InResource));
		}

		void ProcessResources() 
		{
			for (auto It = Resources.CreateIterator(); It; ++It)
			{
				if ((*It).GetRefCount() == 1)
				{
					PendingResources.Add({ *It, CurCpuFrame });
					It.RemoveCurrent();
				}
			}

			for (auto It = PendingResources.CreateIterator(); It; ++It)
			{
				if (CurGpuFrame >= It->WaitFrame)
				{
					It.RemoveCurrent();
				}
			}
		}

	private:
		TSparseArray<TRefCountPtr<GpuResource>> Resources;

		//Resources no longer held by user
		TSparseArray<PendingResource> PendingResources;
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
				GDeferredReleaseManager->AddResource(static_cast<T*>(this));
			}
		}
		virtual ~Dx12DeferredDeleteObject() = default;
	};
}
