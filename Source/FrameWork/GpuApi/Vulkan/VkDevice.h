#pragma once
#include "VkCommon.h"
#include "GpuApi/GpuRhi.h"

namespace FW::VK
{
	inline VkInstance GInstance;
	inline VkDevice GDevice;
	inline VkPhysicalDevice GPhysicalDevice = VK_NULL_HANDLE;
	inline VkQueue GGraphicsQueue;
	inline uint32_t GraphicsQueueIndex;
	inline VmaAllocator GAllocator;
#if PLATFORM_WINDOWS
	constexpr VkExternalMemoryHandleTypeFlagBits ExternalMemoryHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
	constexpr VkExternalSemaphoreHandleTypeFlagBits ExternalSemaphoreHandleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_WIN32_BIT;
#endif
	//inline VkSemaphore GSharedSemaphore;
	//inline HANDLE GSharedSemaphoreHandle;

	class VulkanQuerySet : public GpuQuerySet
	{
	public:
		VulkanQuerySet(uint32 InCount);
		~VulkanQuerySet();
		double GetTimestampPeriodNs() const override;
		void ResolveResults(uint32 FirstQuery, uint32 QueryCount, TArray<uint64>& OutTimestamps) override;
		VkQueryPool GetPool() const { return Pool; }
	private:
		VkQueryPool Pool = VK_NULL_HANDLE;
	};

	extern void InitVulkanCore();
}
