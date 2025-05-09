#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading ShaderConductor..."
    ShaderConductor_Dir="$CurrentPath/Lib"
    ShaderConductor_Url="https://gitee.com/mxr233/shader-helper-dependency/releases/download/v0.1/ShaderConductor-Mac.zip"
    cd "$ShaderConductor_Dir" || exit 1
    curl -LO $ShaderConductor_Url || exit 1
    tar -zxf ShaderConductor-Mac.zip || exit 1
    rm ShaderConductor-Mac.zip
echo
 
exit 0
