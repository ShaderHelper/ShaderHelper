FrameWorkHierarchy = {
    ["Sources/*"] = {"**"},
   -- ["Shaders/*"] = {"%{_WORKING_DIR}/Resource/Shaders/**.hlsl"},
    ["Premake/*"] = {"%{_WORKING_DIR}/Premake/**.lua"}
}

project "FrameWork"
    kind "SharedLib"   
    location "%{_WORKING_DIR}/ProjectFiles"

	addToProjectHierarchy(FrameWorkHierarchy)

    includedirs
    {
        "./",
    }

	defines {
		"FRAMEWORK_API=DLLEXPORT"
	}

    uses {
        "UE", "magic_enum", "Python","ShaderConductor", "shaderc"
    }
	
	addToProjectHierarchy(UeHierarchy)
    addToProjectHierarchy(magic_enumHierarchy)
    addToProjectHierarchy(pythonHierarchy)
    addToProjectHierarchy(shadercHierarchy)

    filter {"files:**/External/UE/** or **/External/Vulkan/**"}
        flags {"ExcludeFromBuild"}
			
    filter {"system:macosx","files:**/Dx12/*.cpp"}
        flags {"ExcludeFromBuild"}

    filter {"system:windows","files:**/Metal/*.cpp"}
        flags {"ExcludeFromBuild"}
    
    filter {"system:linux","files:**/Dx12/*.cpp or **/Metal/*.cpp"}
        flags {"ExcludeFromBuild"}

    filter "system:windows"
        pchheader "CommonHeader.h"
        pchsource "CommonHeader.cpp"
        
        private_uses {
            "AgilitySDK", "WinPixEventRuntime", "Vulkan"
        }
        addToProjectHierarchy(shaderConductorHierarchy)
		addToProjectHierarchy(AgilitySDKHierarchy)
		addToProjectHierarchy(WinPixEventRuntimeHierarchy)
        addToProjectHierarchy(vulkanHierarchy)
		
        links {
            "d3d12", "dxgi", "dxguid"
        }

    filter "system:macosx"
        private_uses { 
            "metal-cpp"
        }
        addToProjectHierarchy(shaderConductorHierarchy)
        addToProjectHierarchy(metalcppHierarchy)
        
        links
        {
            "Cocoa.framework",
            "Metal.framework",
            "CoreVideo.framework",
            "QuartzCore.framework",
            "Foundation.framework",
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



        

