#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading UE..."
    UE_Lib_Dir="$CurrentPath/Lib"
    UE_Lib_Url0="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/27ea04bd1366519f875352f0b3a30f3b/UE-Lib-Mac.zip.00"
    UE_Lib_Url1="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/38eb2bdd6e7ec1f79c2d9ef98624680b/UE-Lib-Mac.zip.01"
    cd "$UE_Lib_Dir" || exit 1
    curl -LO $UE_Lib_Url0 || exit 1
    curl -LO $UE_Lib_Url1 || exit 1
    cat UE-Lib-Mac.zip.* > UE-Lib-Mac.zip || exit 1
    tar -zxf UE-Lib-Mac.zip || exit 1
    rm UE-Lib-Mac.zip*

    UE_Src_Dir="$CurrentPath/Src"
    UE_Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/c54de905c2350d3e380d3464a4a885df/UE-Src.tar"
    cd "$UE_Src_Dir" || exit 1
    curl -LO $UE_Src_Url || exit 1
    tar -zxf UE-Src.tar || exit 1
    rm UE-Src.tar
echo

exit 0
