#pragma once
namespace FRAMEWORK {
	class FRAMEWORK_API PathHelper {
	public:
		static FString ProjectDir();
		static FString ResourceDir();
		static FString ExternalDir();
		static FString SavedDir();
	};
}
