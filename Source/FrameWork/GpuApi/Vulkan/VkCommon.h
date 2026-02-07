#pragma once

#if PLATFORM_WINDOWS
	#pragma warning(push)
	#pragma warning(disable:4191)
	#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "Volk/volk.h"
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#include "vma/vk_mem_alloc.h"

#define VkCheck(Call)                                           \
	do                                                          \
	{                                                           \
		VkResult Result = Call;                                 \
		if(Result != VK_SUCCESS) {                              \
			OutputVkError(Result, #Call, __FILE__, __LINE__);   \
			std::abort();                                       \
		}                                                       \
	} while (0)


#define EnumError(ErrorInfo)						\
	case ErrorInfo : return TEXT(#ErrorInfo);

DECLARE_LOG_CATEGORY_EXTERN(LogVulkan, Log, All);
inline DEFINE_LOG_CATEGORY(LogVulkan);

namespace FW
{
	inline void OutputVkError(VkResult Result, const ANSICHAR* Code, const ANSICHAR* Filename, uint32 Line)
	{
		SH_LOG(LogVulkan, Error, TEXT("VkError(%s) encountered during calling %s.(%s-%u)"), ANSI_TO_TCHAR(magic_enum::enum_name(Result).data()), ANSI_TO_TCHAR(Code), ANSI_TO_TCHAR(Filename), Line);
	}
}

