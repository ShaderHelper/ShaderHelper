FrameWorkHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp", "**.hpp"},
	["Shaders/*"] = {"%{_WORKING_DIR}/Resource/Shaders/**.hlsl"}
}

if os.target() == premake.MACOSX then
	defaultGpuApi = "Metal"
elseif os.target() == premake.WINDOWS then
	defaultGpuApi = "Dx12"
end

newoption {
    trigger = "GpuApi",
	value = "API",
    description = "Choose a gpu api",
	default = defaultGpuApi,
	allowed = {
		{ "Dx12", "Directx12 (Windows only)"},
		{ "Metal", "Metal (Mac only)"},
	}
}

project "FrameWork"
    kind "SharedLib"   
    location "%{_WORKING_DIR}/ProjectFiles"

    includedirs
    {
        "./",
    }

	defines {
		"FRAMEWORK_API=DLLEXPORT"
	}

    uses {
        "UE", "magic_enum"
    }
	
	addToProjectHierarchy(FrameWorkHierarchy);
	addToProjectHierarchy(UeHierarchy);
    addToProjectHierarchy(magic_enumHierarchy);
	
	filter {"files:**/External/UE/Src/**.cpp or **/External/UE/Src/**.mm"}
        flags {"ExcludeFromBuild"}
	
	filter {"options:not GpuApi=Dx12","files:**/Dx12/*.cpp"}
        flags {"ExcludeFromBuild"}
	filter {"options:not GpuApi=Metal","files:**/Metal/*.cpp"}
        flags {"ExcludeFromBuild"}

    filter "system:windows"
        pchheader "CommonHeader.h"
        pchsource "CommonHeader.cpp"
        
        private_uses {
            "DXC", "AgilitySDK", "WinPixEventRuntime"
        }
		addToProjectHierarchy(AgilitySDKHierarchy);
		addToProjectHierarchy(DXCHierarchy);
		addToProjectHierarchy(WinPixEventRuntimeHierarchy);
		
        links {
            "d3d12", "dxgi", "dxguid"
        }

    filter "system:macosx"
        private_uses { 
            "MtlppUE", "ShaderConductor"
        }
        links
        {
            "Cocoa.framework",
            "Metal.framework",
            "CoreVideo.framework",
        }

usage "FrameWork"
    --Note that "DLLIMPORT" macro is defined by UE module. (see MacPlatform.h, WindowsPlatform.h)
	defines {
		"FRAMEWORK_API=DLLIMPORT"
	}
	includedirs
    {
        path.getabsolute("./"),
    }



        

