UnitTestGpuApiHierarchy = {
    ["Sources/*"] = {"**"},
}

project "UnitTestGpuApi"
    kind "WindowedApp"   
    location "%{_WORKING_DIR}/ProjectFiles"

    addToProjectHierarchy(UnitTestGpuApiHierarchy)

    includedirs
    {
        "./",
    }

    uses {
		"Framework",
	}

    filter "system:windows"
        files {"%{_WORKING_DIR}/Resource/Misc/Windows/*"}
        vpaths {["Resource"] = "%{_WORKING_DIR}/Resource/Misc/Windows/*"}
        pchheader "CommonHeader.h"
        pchsource "CommonHeader.cpp"

    filter "system:macosx"
        files {"%{_WORKING_DIR}/Resource/Misc/Mac/*"}
        vpaths {["Resource"] = "%{_WORKING_DIR}/Resource/Misc/Mac/*"}



        

