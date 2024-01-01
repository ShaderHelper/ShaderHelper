UnitTestUtilHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp"},
}

project "UnitTestUtil"
    kind "WindowedApp"   
    location "%{_WORKING_DIR}/ProjectFiles"

    vpaths(UnitTestUtilHierarchy)

    files {seq(UnitTestUtilHierarchy)}

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



        

