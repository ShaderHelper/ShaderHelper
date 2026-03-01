#pragma once
#include "GpuApi/GpuResource.h"
#include "VkCommon.h"

namespace FW
{
	class VkDeferredReleaseManager
	{
	public:
		void AddResource(TRefCountPtr<GpuResource> InResource)
		{
			FScopeLock Lock(&CS);
			Resources.Add(MoveTemp(InResource));
		}

		void ProcessResources()
		{
			FScopeLock Lock(&CS);
			for (auto It = Resources.CreateIterator(); It; ++It)
			{
				if ((*It).GetRefCount() == 1)
				{
					It.RemoveCurrent();
				}
			}
		}

		const auto& GetResources() const { return Resources; }

	private:
		TSparseArray<TRefCountPtr<GpuResource>> Resources;
		FCriticalSection CS;
	};

	inline VkDeferredReleaseManager GVkDeferredReleaseManager;

	void SetVkObjectName(VkObjectType InObjectType, uint64 InObjectHandle, const FString& InName);
}
