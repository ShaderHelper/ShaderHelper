FrameWorkHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp", "**.hpp"},
	["Resource/Shaders/*"] = {"%{_WORKING_DIR}/Resource/Shaders/**.hlsl"}
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



        

