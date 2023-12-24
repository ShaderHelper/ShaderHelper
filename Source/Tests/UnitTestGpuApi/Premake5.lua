UnitTestGpuApiHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp"},
}

project "UnitTestGpuApi"
    kind "WindowedApp"   
    location "%{_WORKING_DIR}/ProjectFiles"

    vpaths(UnitTestGpuApiHierarchy)

    files {seq(UnitTestGpuApiHierarchy)}

    includedirs
    {
        "./",
    }

    uses {
		"UE",
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



        

