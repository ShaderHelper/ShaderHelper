# ShaderHelper (WIP)

This is a shader editor based on some special modules from UnrealEngine (Slate, ImageWrapper ...). The engine source code is the 5.0.3 version, with only a few changes.

<p align="center">
<img src="https://github.com/SjMxr233/ShaderHelper/blob/main/ScreenShot/App.png" width="640" height="360">

## Build Instructions

### Minimum Windows Requirements
[![Windows](https://github.com/mxrhyx233/ShaderHelper/actions/workflows/Windows.yml/badge.svg)](https://github.com/mxrhyx233/ShaderHelper/actions/workflows/Windows.yml)
* Windows 10 64-bit version 1909
* x86_64
* Visual studio 2022

First run `downloadDep.bat` to get external dependencies, then open the `.sln` file created after running `bootstrap.bat` and build the project.  

### Minimum MacOS Requirements
[![MacOS](https://github.com/mxrhyx233/ShaderHelper/actions/workflows/MacOS.yml/badge.svg)](https://github.com/mxrhyx233/ShaderHelper/actions/workflows/MacOS.yml)
* Sonoma 14.5
* x86_64 or arm64
* Xcode 16

First run `downloadDep.sh` to get external dependencies, then open the `.xcworkspace` file created after running `bootstrap.sh` and build the project.
