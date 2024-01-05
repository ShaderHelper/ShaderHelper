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



        

