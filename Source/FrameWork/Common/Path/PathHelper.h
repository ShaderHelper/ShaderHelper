#pragma once
namespace FRAMEWORK {

	class FRAMEWORK_API PathHelper {
	public:
		// ShaderHelper/
		static FString WorkspaceDir();
		// ShaderHelper/Resource
		static FString ResourceDir();
		static FString ExternalDir();
		static FString SavedDir();
		static FString SavedLogDir();
		static FString SavedShaderDir();
		static FString SavedConfigDir();
        static FString SavedCaptureDir();
		static FString ShaderDir();
		static FString ErrorDir();
	};
}
