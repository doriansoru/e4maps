#!/bin/bash

# Script to update translation files for e4maps
# This script will:
# 1. Extract translatable strings from source code to create/update the POT template
# 2. Merge new strings with existing PO files to preserve existing translations
# 3. Generate updated MO files

set -e  # Exit on any error

echo "Updating translation files for e4maps..."

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check if po directory exists
if [ ! -d "po" ]; then
    echo "Error: po directory not found!"
    exit 1
fi

# Create or update the POT file (translation template)
echo "Extracting translatable strings..."

# Find all source files to extract translations from
SOURCE_FILES=$(find src/ -name "*.cpp" -o -name "*.hpp" -o -name "*.h" | tr '\n' ' ')

if [ -z "$SOURCE_FILES" ]; then
    echo "Error: No source files found!"
    exit 1
fi

# Create POT file with xgettext
xgettext --package-name="e4maps" \
         --package-version="1.0.0" \
         --copyright-holder="Dorian Soru" \
         --msgid-bugs-address="doriansoru@gmail.com" \
         --keyword=_ --keyword=N_ \
         --from-code=UTF-8 \
         --output="po/e4maps.pot" \
         $SOURCE_FILES

echo "Translation template updated: po/e4maps.pot"

# Update each existing PO file with new strings
for po_file in po/*.po; do
    if [ -f "$po_file" ]; then
        lang_code=$(basename "$po_file" .po)
        echo "Updating translation file for $lang_code: $po_file"
        
        # Use msgmerge to update the PO file with new strings
        # --update modifies the PO file in place
        # --backup=none prevents creation of backup files
        msgmerge --update --backup=none "$po_file" po/e4maps.pot
        
        # Generate the MO file from the updated PO file
        mo_dir="po/$lang_code/LC_MESSAGES"
        mkdir -p "$mo_dir"
        msgfmt "$po_file" -o "$mo_dir/e4maps.mo"
        
        echo "Generated: $mo_dir/e4maps.mo"
    fi
done

echo
echo "Translation files updated successfully!"
echo
echo "To add new languages, create a new .po file (e.g., fr.po, de.po) in the po/ directory"
echo "and then run this script again. The script will automatically process all .po files."
echo
echo "New strings will appear as \"\" in the PO files and will need to be translated manually."