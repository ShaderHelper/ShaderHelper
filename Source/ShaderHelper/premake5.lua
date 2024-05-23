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
		"FrameWork",
	}

    addPrivateIncludeDir_ue("Slate")

    filter "system:windows"
        files {"%{_WORKING_DIR}/Resource/Misc/Windows/*"}
        vpaths {["Resource"] = "%{_WORKING_DIR}/Resource/Misc/Windows/*"}
        pchheader "CommonHeader.h"
        pchsource "CommonHeader.cpp"

    filter "system:macosx"
        files {"%{_WORKING_DIR}/Resource/Misc/Mac/*"}
        vpaths {["Resource"] = "%{_WORKING_DIR}/Resource/Misc/Mac/*"}

    filter "system:linux"
        files {"%{_WORKING_DIR}/Resource/Misc/Linux/*"}
        vpaths {["Resource"] = "%{_WORKING_DIR}/Resource/Misc/Linux/*"}
