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
    ShaderConductor_Dir="$CurrentPath/External/ShaderConductor/Lib"
    ShaderConductor_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/3a23af053e9a4073a3f2821ec33d43c4/ShaderConductor-Mac.zip"
    cd "$ShaderConductor_Dir"
    curl -LO $ShaderConductor_Url || Error
    tar -zxf ShaderConductor-Mac.zip || Error
    rm ShaderConductor-Mac.zip
echo

echo "Downloading UE..."
    UE_Lib_Dir="$CurrentPath/External/UE/Lib"
    UE_Lib_Url0="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/3bcaab5a1760606521b5d809aea1e1db/UE-Lib-Mac.zip.00"
    UE_Lib_Url1="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/57fcc7c09d8da453e7821d32d5d18053/UE-Lib-Mac.zip.01"
    cd "$UE_Lib_Dir"
    curl -LO $UE_Lib_Url0 || Error
    curl -LO $UE_Lib_Url1 || Error
    cat UE-Lib-Mac.zip.* > UE-Lib-Mac.zip || Error
    rm -rf UE.dSYM
    tar -zxf UE-Lib-Mac.zip || Error
    rm UE-Lib-Mac.zip*

    UE_Src_Dir="$CurrentPath/External/UE/Src"
    UE_Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/c0c6af00cb2edf593ac76e9ca9135e2f/UE-Src.zip"
    cd "$UE_Src_Dir"
    curl -LO $UE_Src_Url || Error
    tar -zxf UE-Src.zip || Error
    rm UE-Src.zip

    echo "Processing UE.dSYM..."
    python3 "$CurrentPath/dsymPlist.py" --dsymFilePath "$UE_Lib_Dir/UE.dSYM" --srcDir "$UE_Src_Dir" --buildSrcDir "/UePlaceholder" || Error
echo

echo "Script Complete"
