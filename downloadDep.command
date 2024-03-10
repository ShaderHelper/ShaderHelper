#!/bin/sh
echo
CurrentPath="$(cd "$(dirname "$0")" && pwd)"
cd $CurrentPath

function Error() {
    echo "Erros encountered during execution."
    echo
    exit 1
}

echo "Downloading ShaderConductor..."
    ShaderConductor_Dir=$CurrentPath/External/ShaderConductor/Lib
    ShaderConductor_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/3a23af053e9a4073a3f2821ec33d43c4/ShaderConductor-Mac.zip"
    cd $ShaderConductor_Dir
    curl -LO $ShaderConductor_Url || Error
    tar -zxf ShaderConductor-Mac.zip || Error
    rm ShaderConductor-Mac.zip
echo

echo "Downloading UE-lib..."
    UE_Lib_Dir=$CurrentPath/External/UE/Lib
    UE_Lib_Url0="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/335e7bb11f5e5a7a06aa0e58259ddf06/UE-Lib-Mac.zip.00"
    UE_Lib_Url1="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/f99bc2c455183ce718cea8cb51bf3e75/UE-Lib-Mac.zip.01"
    cd $UE_Lib_Dir
    curl -LO $UE_Lib_Url0 || Error
    curl -LO $UE_Lib_Url1 || Error
    cat UE-Lib-Mac.zip.* > UE-Lib-Mac.zip || Error
    tar -zxf UE-Lib-Mac.zip || Error
    rm UE-Lib-Mac.zip*
echo

echo "Script Complete"
