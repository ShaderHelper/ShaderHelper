#pragma once
#include "VkCommon.h"

namespace FW
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
	extern void InitVulkanCore();
}