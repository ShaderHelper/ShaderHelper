#!/bin/sh

cur=$(dirname "$(readlink -f "$0")")
LldbPath=~/.lldbinit
echo "settings set target.inline-breakpoint-strategy always" > $LldbPath
echo "command script import \"$cur/UEDataFormatters_2ByteChars.py\"" >> $LldbPath