#pragma once
namespace FW {

	class FRAMEWORK_API PathHelper {
	public:
		// ShaderHelper/
		static FString WorkspaceDir();
		// ShaderHelper/Resource
		static FString ResourceDir();
		static FString PluginDir();
		static FString ExternalDir();
		static FString SavedDir();
		static FString SavedLogDir();
		static FString SavedShaderDir();
		static FString SavedConfigDir();
        static FString SavedCaptureDir();
		static FString ShaderDir();
		static FString ErrorDir();
        
        //Support the project path with space
        static bool ParseProjectPath(const FString& CommandLine, FString& OutPath);
	};
}
