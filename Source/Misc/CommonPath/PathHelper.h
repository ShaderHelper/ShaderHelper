#pragma once
#include "CommonHeader.h"

namespace SH {
	class PathHelper {
	public:
		static FString ProjectDir();
		static FString ResourceDir();
		static FString ExternalDir();
	};
}