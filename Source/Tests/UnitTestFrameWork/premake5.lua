UnitTestFrameWorkHierarchy = {
    ["Sources/*"] = {"**.h","**.cpp"},
}

project "UnitTestFrameWork"
    kind "WindowedApp"   
    location "%{_WORKING_DIR}/ProjectFiles"

	pchheader "CommonHeader.h"
	pchsource "CommonHeader.cpp"

    vpaths(UnitTestFrameWorkHierarchy)

    files {seq(UnitTestFrameWorkHierarchy)}

    includedirs
    {
        "./",
    }

    uses {
		"UE",
		"Framework",
	}




        

