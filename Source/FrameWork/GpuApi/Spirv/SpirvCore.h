#pragma once

namespace FW
{
	using SpvId = uint32;

	class SpvType
	{
		
	};

	enum class SpvOp
	{
		String = 7,
		ExtInst = 12,
		ExecutionMode = 16,
		TypeFloat = 22,
		Constant = 43,
	};

	enum class SpvExecutionMode
	{
		LocalSize = 17,
	};
}
