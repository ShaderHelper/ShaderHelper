#!/bin/sh
echo
CurrentPath="$(cd "$(dirname "$0")" && pwd)"
cd "$CurrentPath"

Error() {
    echo "Erros encountered during execution."
    echo
    exit 1
}

OS_TYPE=$(uname)
case "$OS_TYPE" in
    Linux)
        PremakePath="$CurrentPath/Premake/premake5_linux"
        if [ ! -f "$PremakePath" ]; then
            echo "ERROR: Could not find premake5, please check the path: \"$PremakePath\""
            Error
        fi

        echo "Premake5: Processing..."
        chmod +x "$PremakePath"
        "$PremakePath" gmake || Error
        ;;
    Darwin)
        PremakePath="$CurrentPath/Premake/Premake5"
        if [ ! -f "$PremakePath" ]; then
            echo "ERROR: Could not find premake5, please check the path: \"$PremakePath\""
            Error
        fi

        xattr -d com.apple.quarantine "$PremakePath" 2>/dev/null

        echo "Premake5: Processing..."

        if [ "$#" -eq 1 ] && [ "$1" == "-UniversalBinary" ]; then
            "$PremakePath" --UniversalBinary xcode4 || Error
        else
            "$PremakePath" xcode4 || Error
        fi
        ;;
    *)
        Error
esac

echo "Premake5 Complete"
