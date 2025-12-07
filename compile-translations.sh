#!/bin/bash

# Script to compile PO translation files to MO files for e4maps
# This script compiles all .po files in the po/ directory to .mo files
# without extracting new strings from source code (useful when only updating translations)

set -e  # Exit on any error

echo "Compiling PO files to MO files for e4maps..."

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check if po directory exists
if [ ! -d "po" ]; then
    echo "Error: po directory not found!"
    exit 1
fi

# Compile each PO file to its corresponding MO file
for po_file in po/*.po; do
    if [ -f "$po_file" ]; then
        lang_code=$(basename "$po_file" .po)
        echo "Compiling translation for $lang_code: $po_file"
        
        # Create the directory structure for MO files
        mo_dir="po/$lang_code/LC_MESSAGES"
        mkdir -p "$mo_dir"
        
        # Compile the PO file to MO using msgfmt
        msgfmt "$po_file" -o "$mo_dir/e4maps.mo"
        
        echo "Generated: $mo_dir/e4maps.mo"
    fi
done

echo
echo "PO files compiled to MO files successfully!"
echo
echo "The MO files are now ready to be used by the application."
echo "If you have updated translations in the PO files, they are now available in the MO files."