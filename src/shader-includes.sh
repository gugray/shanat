#!/bin/bash

pushd "$1" > /dev/null || { echo "Failed to cd into $1"; exit 1; }

awk '
# Check if the line starts with SRC
/^[[:space:]]*SRC[[:space:]]+/ {
    # extract the filename (everything after SRC and spaces)
    sub(/^[[:space:]]*SRC[[:space:]]+/, "", $0)
    filename = $0
    while ((getline line < filename) > 0) print line
    next
}
# Otherwise print the line as-is
{ print }
' shader_template.h > shaders.h

popd > /dev/null
