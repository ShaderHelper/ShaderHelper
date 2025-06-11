#pragma once

namespace FW::MessageDialog
{
	enum MessageType
	{
		Ok =  1 << 0,
		No = 1 << 1,
		Cancel = 1 << 2,
		
		OkCancel = Ok | Cancel,
		OkNoCancel = Ok | No | Cancel,
	};

	enum class MessageRet
	{
		Ok,
		No,
		Cancel,
	};

	FRAMEWORK_API MessageRet Open(MessageType MsgType, TSharedPtr<SWindow> Parent, const TAttribute<FText>& InMessage);
	
}
