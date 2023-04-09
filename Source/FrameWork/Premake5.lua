FrameWorkHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp", "**.hpp"},
    ["External/d3dx12"] = {"../../External/AgilitySDK/Inc/d3dx12.h"},
    ["External/MtlppUE"] = {
        "../../External/MtlppUE/src/*.hpp", 
        "../../External/MtlppUE/src/*.mm", 
        "../../External/MtlppUE/src/*.inl"
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
        "UE"
    }

    filter "system:windows"
        pchheader "CommonHeader.h"
        pchsource "CommonHeader.cpp"
        
        private_uses {
            "D3D12MemoryAllocator",
            "DXC", "AgilitySDK", "WinPixEventRuntime"
        }
    
        links {
            "d3d12", "dxgi", "dxguid"
        }

    filter "system:macosx"
        private_uses { "MtlppUE" }

usage "FrameWork"
    --Note that "DLLIMPORT" macro is defined by UE module. (see MacPlatform.h, WindowsPlatform.h)
	defines {
		"FRAMEWORK_API=DLLIMPORT"
	}
	includedirs
    {
        path.getabsolute("./"),
    }



        

