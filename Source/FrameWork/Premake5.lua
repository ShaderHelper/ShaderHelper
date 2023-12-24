FrameWorkHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp", "**.hpp"},
    ["External/d3dx12"] = {"%{_WORKING_DIR}/External/AgilitySDK/Inc/d3dx12.h"},
    ["External/magic_enum"] = {"%{_WORKING_DIR}/External/magic_enum/magic_enum.hpp"},
    ["External/MtlppUE"] = {
        "%{_WORKING_DIR}/External/MtlppUE/src/*.hpp", 
        "%{_WORKING_DIR}/External/MtlppUE/src/*.mm", 
        "%{_WORKING_DIR}/External/MtlppUE/src/*.inl"
    },
	["Resource/Shaders"] = {"%{_WORKING_DIR}/Resource/Shaders/*"}
}

project "FrameWork"
    kind "SharedLib"   
    location "%{_WORKING_DIR}/ProjectFiles"

    vpaths(FrameWorkHierarchy)

    files {seq(FrameWorkHierarchy)}

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

    filter "system:windows"
        pchheader "CommonHeader.h"
        pchsource "CommonHeader.cpp"
        
        private_uses {
            "DXC", "AgilitySDK", "WinPixEventRuntime"
        }
    
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



        

