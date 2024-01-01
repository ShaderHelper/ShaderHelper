ShaderHelperHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp"},
}

project "ShaderHelper"
    kind "WindowedApp"   
    location "%{_WORKING_DIR}/ProjectFiles"

    vpaths(ShaderHelperHierarchy)

    files {seq(ShaderHelperHierarchy)}

    includedirs
    {
        "./",
    }

    uses {
		"UE",
		"FrameWork",
	}

    filter "system:windows"
        files {"%{_WORKING_DIR}/Resource/Misc/Windows/*"}
        vpaths {["Resource"] = "%{_WORKING_DIR}/Resource/Misc/Windows/*"}
        pchheader "CommonHeader.h"
        pchsource "CommonHeader.cpp"

    filter "system:macosx"
        files {"%{_WORKING_DIR}/Resource/Misc/Mac/*"}
        vpaths {["Resource"] = "%{_WORKING_DIR}/Resource/Misc/Mac/*"}
