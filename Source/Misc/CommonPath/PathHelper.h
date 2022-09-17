#pragma once
#include "CommonHeaderForUE.h"

namespace SH {
	class PathHelper {
	public:
		static FString ProjectDir();
		static FString ResourceDir();
		static FString ExternalDir();
	};
}