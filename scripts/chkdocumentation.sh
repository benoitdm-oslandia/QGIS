#!/usr/bin/env bash
# shellcheck disable=SC2164
###########################################################################
#    chkdocumentation.sh
#    ---------------------
#    Date                 : December 2025
#    Copyright            : (C) 2025 Oslandia
###########################################################################
#                                                                         #
#   This program is free software; you can redistribute it and/or modify  #
#   it under the terms of the GNU General Public License as published by  #
#   the Free Software Foundation; either version 2 of the License, or     #
#   (at your option) any later version.                                   #
#                                                                         #
###########################################################################

#
# Build a docker image with the same Doxygen version than CI
# and run ./scripts/documentation_check/documentation_check.sh on whole qgis directory
#

PROG_NAME=$(basename $0)

set -E
set -o functrace
function handle_error {
    local retval=$?
    local line=${last_lineno:-$1}
    echo "Failed at $PROG_NAME:$line $BASH_COMMAND"
    exit $retval
}
trap 'handle_error $LINENO ${BASH_LINENO[@]}' ERR

SOURCE_DIR=$(dirname $0)/..
SOURCE_DIR=$(realpath $SOURCE_DIR)
DOC_SCRIPT=$SOURCE_DIR/scripts/documentation_check

echo "== Build docker image"
docker build -t qgis/doc_check:latest -f "$DOC_SCRIPT/Dockerfile" "$DOC_SCRIPT/"

echo "== Run docker image with '$SOURCE_DIR'"
docker run --rm -ti -v "$SOURCE_DIR:/qgis" qgis/doc_check:latest

echo "== Done!"
