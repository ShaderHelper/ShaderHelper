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
			Resources.Add(MoveTemp(InResource));
		}

		void ProcessResources()
		{
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
	};

	inline VkDeferredReleaseManager GVkDeferredReleaseManager;
}
