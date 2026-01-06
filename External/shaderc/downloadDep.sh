#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading shaderc..."
    shaderc_Dir="$CurrentPath/Lib"
    shaderc_Url="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/shaderc-Mac.zip"
    cd "$shaderc_Dir" || exit 1
    curl -LO $shaderc_Url || exit 1
    tar -zxf shaderc-Mac.zip || exit 1
    rm shaderc-Mac.zip
echo
 
exit 0
