# ShaderHelper (WIP)

This is a shader editor based on some special modules from UnrealEngine (Slate, ImageWrapper ...). The engine source code is the 5.0.3 version, with only a few changes.

<p align="center">
<img src="https://github.com/SjMxr233/ShaderHelper/blob/main/ScreenShot/App.png" width="640" height="360">

## Features

* TODO

## Coding Standard

* Use `<>` syntax to include the headers from UnrealEngine.
* Custom ui should use the same naming conventions as UnrealEngine.

## Build Instructions

### Windows Requirements
[![Windows](https://github.com/mxrhyx233/ShaderHelper/actions/workflows/Windows.yml/badge.svg)](https://github.com/mxrhyx233/ShaderHelper/actions/workflows/Windows.yml)
* Windows 10 (64-bit) - Version 1909 or later
* x86_64
* Visual studio 2019 or 2022

First run `downloadDep.bat` to get external dependencies, then open the `.sln` file created after running `bootstrap.bat` and build the project.  
> Note: `bootstrap.bat` generates project files corresponding to the latest version of visual studio you have installed by default. If you need an older version, specify it manually like this: `bootstrap.bat vs20XX`

### MacOS Requirements
[![MacOS](https://github.com/mxrhyx233/ShaderHelper/actions/workflows/MacOS.yml/badge.svg)](https://github.com/mxrhyx233/ShaderHelper/actions/workflows/MacOS.yml)
* 10.15 system version at least
* x86_64 or arm64
* Xcode

First run `downloadDep.command` to get external dependencies, then open the `.xcworkspace` file created after running `bootstrap.command` and build the project.
