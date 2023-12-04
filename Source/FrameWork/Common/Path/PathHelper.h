#pragma once
namespace FRAMEWORK {
	class FRAMEWORK_API PathHelper {
	public:
		static FString InitialDir();
		static FString ResourceDir();
		static FString ExternalDir();
		static FString SavedDir();
		static FString SavedShaderDir();
		static FString SavedConfigDir();
	};
}
