#pragma once

namespace FW
{
	using SpvId = uint32;

	enum class SpvOp
	{
		String = 7,
		ExtInst = 12,
		ExecutionMode = 16,
	};

	enum class SpvExecutionMode
	{
		LocalSize = 17,
	};
}
