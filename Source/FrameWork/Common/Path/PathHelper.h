#pragma once
namespace FRAMEWORK {

	class FRAMEWORK_API PathHelper {
	public:
		// ShaderHelper/
		static FString WorkSpaceDir();
		// ShaderHelper/Resource
		static FString ResourceDir();
		static FString ExternalDir();
		static FString SavedDir();
		static FString SavedShaderDir();
		static FString SavedConfigDir();

		static FString ShaderDir();
	};
}
