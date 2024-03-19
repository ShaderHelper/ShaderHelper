#!/bin/sh
echo
CurrentPath="$(cd "$(dirname "$0")" && pwd)"
cd "$CurrentPath"

function Error() {
    echo "Erros encountered during execution."
    echo
    exit 1
}

echo "Downloading ShaderConductor..."
    UE_Src_Dir="$CurrentPath/Src"
    UE_Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/c0c6af00cb2edf593ac76e9ca9135e2f/UE-Src.zip"
    cd "$UE_Src_Dir"
    curl -LO $UE_Src_Url || Error
    tar -zxf UE-Src.zip || Error
    rm UE-Src.zip
echo

echo "Script Complete"
