#pragma once
#include "CommonHeader.h"

namespace FRAMEWORK {
	class PathHelper {
	public:
		static FString ProjectDir();
		static FString ResourceDir();
		static FString ExternalDir();
	};
}