#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading UE..."
    UE_Lib_Dir="$CurrentPath/Lib"
    UE_Lib_Url0="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/495c2cdecda5d331caa9cd9d785337fa/UE-Lib-Mac.zip.00"
    UE_Lib_Url1="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/f751adc81d2e2d544bd59267cc8e5734/UE-Lib-Mac.zip.01"
    cd "$UE_Lib_Dir" || exit 1
    curl -LO $UE_Lib_Url0 || exit 1
    curl -LO $UE_Lib_Url1 || exit 1
    cat UE-Lib-Mac.zip.* > UE-Lib-Mac.zip || exit 1
    tar -zxf UE-Lib-Mac.zip || exit 1
    rm UE-Lib-Mac.zip*

    UE_Src_Dir="$CurrentPath/Src"
    UE_Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/1607e8c444f77ead08ee852853c530fe/UE-Src.zip"
    cd "$UE_Src_Dir" || exit 1
    curl -LO $UE_Src_Url || exit 1
    tar -zxf UE-Src.zip || exit 1
    rm UE-Src.zip
echo

exit 0