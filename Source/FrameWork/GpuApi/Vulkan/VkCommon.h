#pragma once

#pragma warning(push)
#pragma warning(disable:4191)
#include "Volk/volk.h"
#pragma warning(pop)

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

