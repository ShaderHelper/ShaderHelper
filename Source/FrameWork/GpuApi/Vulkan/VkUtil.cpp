#include "CommonHeader.h"
#include "VkUtil.h"
#include "VkDevice.h"

namespace FW
{
    void SetVkObjectName(VkObjectType InObjectType, uint64 InObjectHandle, const FString& InName)
	{
#if GPU_API_DEBUG
		FTCHARToUTF8 Utf8Name(*InName);
		VkDebugUtilsObjectNameInfoEXT NameInfo{
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
			.objectType = InObjectType,
			.objectHandle = InObjectHandle,
			.pObjectName = Utf8Name.Get()
		};
		vkSetDebugUtilsObjectNameEXT(GDevice, &NameInfo);
#endif
	}
}
