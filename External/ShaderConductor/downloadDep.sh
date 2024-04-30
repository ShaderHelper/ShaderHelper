#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading ShaderConductor..."
    ShaderConductor_Dir="$CurrentPath/Lib"
    ShaderConductor_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/3a23af053e9a4073a3f2821ec33d43c4/ShaderConductor-Mac.zip"
    cd "$ShaderConductor_Dir" || exit 1
    curl -LO $ShaderConductor_Url || exit 1
    tar -zxf ShaderConductor-Mac.zip || exit 1
    rm ShaderConductor-Mac.zip
echo

exit 0
