#!/bin/sh
echo
CurrentPath="$(cd "$(dirname "$0")" && pwd)"
cd $CurrentPath

function Error() {
    echo "Erros encountered during execution."
    echo
    exit 1
}

PremakePath=$CurrentPath/Premake/Premake5
if [ ! -f $PremakePath ]; then
    echo "ERROR: Could not find premake5, please check the path: $PremakePath"
    Error
fi

xattr -d com.apple.quarantine $PremakePath 2>/dev/null

echo "Premake5: Processing..."

if [ "$#" -eq 1 ] && [ "$1" == "-UniversalBinary" ]; then
    $PremakePath --UniversalBinary xcode4 || Error
else
    $PremakePath xcode4 || Error
fi

echo "Premake5 Complete"

echo "Generate LLDB Visualizers..."
if [ -f ~/.lldbinit ]; then
    rm -f ~/.lldbinit
fi
echo 'echo -n "settings set target.inline-breakpoint-strategy always\n"' | bash > ~/.lldbinit
echo 'echo -n "command script import \"`pwd`/External/UE/UEDataFormatters_2ByteChars.py\"\n"' | bash >> ~/.lldbinit
echo "Generate Complete"
