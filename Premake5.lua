workspace "AnimTester"  
    architecture "x64"
    configurations { "Debug", "Release" } 




project "name"  
kind "ConsoleApp"   
language "C++"   
targetdir "bin/%{cfg.buildcfg}" 
files { "**.hpp", "**.cpp" } 
filter "configurations:Debug"
defines { "DEBUG" }  
symbols "On" 
filter "configurations:Release"  
defines { "NDEBUG" }    
optimize "On" 