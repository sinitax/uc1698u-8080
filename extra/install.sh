#!/bin/bash

REPOROOT=$(git rev-parse --show-toplevel)
TARGETDIR="$HOME/Arduino/libraries/uc1698u-8080"

[ ! -e "$(dirname $TARGETDIR)" ] && mkdir -p "$(dirname $TARGETDIR)"

if [ -e "$TARGETDIR" ]; then
    echo "already installed.. remove '$TARGETDIR' to uninstall"
    exit 1
fi

cp -r "$REPOROOT/lib" "$TARGETDIR"
