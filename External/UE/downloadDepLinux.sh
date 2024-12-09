#!/bin/sh
CurrentPath="$(cd "$(dirname "$0")" && pwd)"

echo "Downloading UE..."
    UE_Lib_Dir="$CurrentPath/Lib"
    UE_Lib_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/472ff39cf6d8eca1c6e594cc248e92e3/UE-Lib-Linux.tar"
    cd "$UE_Lib_Dir" || exit 1
    wget $UE_Lib_Url || exit 1
    tar -zxf UE-Lib-Linux.tar || exit 1
    rm UE-Lib-Linux.tar

    UE_Src_Dir="$CurrentPath/Src"
    UE_Src_Url="https://gitlab.com/mxrhyx/shaderhelperdependency/uploads/c54de905c2350d3e380d3464a4a885df/UE-Src.tar"
    cd "$UE_Src_Dir" || exit 1
    wget $UE_Src_Url || exit 1
    tar -zxf UE-Src.tar || exit 1
    rm UE-Src.tar
echo

exit 0
