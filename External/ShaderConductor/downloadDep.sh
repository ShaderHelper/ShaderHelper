#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading ShaderConductor..."
    ShaderConductor_Dir="$CurrentPath/Lib"
    ShaderConductor_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/a63d78d3904cdb31cffacc0a4acdcfd7/ShaderConductor-Mac.zip"
    cd "$ShaderConductor_Dir" || exit 1
    curl -LO $ShaderConductor_Url || exit 1
    tar -zxf ShaderConductor-Mac.zip || exit 1
    rm ShaderConductor-Mac.zip
echo
 
exit 0
