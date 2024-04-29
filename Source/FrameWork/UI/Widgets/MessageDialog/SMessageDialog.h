#pragma once

namespace FRAMEWORK::MessageDialog
{
	enum MessageType
	{
		Ok,
		OkCancel,
	};

	FRAMEWORK_API bool Open(MessageType MsgType, const FText& InMessage);
	
}
