ShaderHelperHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp"},
}

project "ShaderHelper"
    kind "WindowedApp"   
    location "%{_WORKING_DIR}/ProjectFiles"

	pchheader "CommonHeader.h"
	pchsource "CommonHeader.cpp"

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
        files {"%{_WORKING_DIR}/Resource/ExeIcon/Windows/*"}
        vpaths {["Resources"] = "%{_WORKING_DIR}/Resource/ExeIcon/Windows/*"}

    filter "system:macosx"
        files {"%{_WORKING_DIR}/Resource/ExeIcon/Mac/*"}
        vpaths {["Resources"] = "%{_WORKING_DIR}/Resource/ExeIcon/Mac/*"}

