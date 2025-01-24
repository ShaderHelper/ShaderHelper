#pragma once

namespace FW::MessageDialog
{
	enum MessageType
	{
		Ok,
		OkCancel,
	};

	FRAMEWORK_API bool Open(MessageType MsgType, TSharedPtr<SWindow> Parent, const TAttribute<FText>& InMessage);
	
}
