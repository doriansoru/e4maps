#!/bin/bash

# Script per creare una directory di distribuzione completa per e4maps
# con tutte le DLL dipendenze necessarie per Windows

set -e  # Esce se un comando fallisce

APP_NAME="e4maps"

# Rilevamento automatico directory di build
if [ -d "./build" ] && [ -f "./build/$APP_NAME.exe" ]; then
    BUILD_DIR="./build"
    echo "Rilevata build in ./build (CI/GitHub Actions)"
elif [ -d "./buildWin" ] && [ -f "./buildWin/$APP_NAME.exe" ]; then
    BUILD_DIR="./buildWin"
    echo "Rilevata build in ./buildWin (Locale)"
else
    echo "ERRORE: Directory di build o eseguibile non trovato."
    echo "Assicurati di aver compilato il progetto."
    exit 1
fi

INSTALL_DIR="./distWin"
DEPS_DIR="$INSTALL_DIR" # Metto le DLL nella root per semplicità di Windows

echo "Creazione della directory di distribuzione per $APP_NAME in $INSTALL_DIR..."

# Pulisci e ricrea
rm -rf "$INSTALL_DIR"
mkdir -p "$INSTALL_DIR"

# Copiare l'eseguibile principale
cp "$BUILD_DIR/$APP_NAME.exe" "$INSTALL_DIR/"
echo "Eseguibile copiato."

# Ottieni tutte le dipendenze DLL usando ldd
# Nota: Questo funziona bene dentro la shell MSYS2/MinGW64
echo "Raccolta delle dipendenze DLL..."
ldd "$BUILD_DIR/$APP_NAME.exe" | grep -o '/mingw64/bin/[^ ]*\.dll' | sort | uniq | while read -r dll_path; do
    if [ -f "$dll_path" ]; then
        cp "$dll_path" "$INSTALL_DIR/"
        # echo "Copiata DLL: $(basename "$dll_path")"
    fi
done

echo "DLL copiate."

# --- Gestione Risorse GTK ---
echo "Copia risorse GTK (Schemi, Icone, Temi)..."

# Schemi GSettings (essenziali per evitare crash all'avvio)
mkdir -p "$INSTALL_DIR/share/glib-2.0/schemas"
if [ -d "/mingw64/share/glib-2.0/schemas" ]; then
    cp /mingw64/share/glib-2.0/schemas/gschemas.compiled "$INSTALL_DIR/share/glib-2.0/schemas/" 2>/dev/null || \
    cp /mingw64/share/glib-2.0/schemas/*.xml "$INSTALL_DIR/share/glib-2.0/schemas/"
    # Se abbiamo copiato gli XML, proviamo a compilarli se glib-compile-schemas esiste
    if [ -f "$INSTALL_DIR/share/glib-2.0/schemas/gschemas.compiled" ]; then
        : # Già compilato
    elif command -v glib-compile-schemas >/dev/null; then
        glib-compile-schemas "$INSTALL_DIR/share/glib-2.0/schemas/"
    fi
fi

# Icone (Adwaita/Hicolor) - Ridotto per risparmiare spazio, ma necessario per UI
mkdir -p "$INSTALL_DIR/share/icons"
for theme in Adwaita hicolor; do
    if [ -d "/mingw64/share/icons/$theme" ]; then
        # Copia solo index.theme e cartelle scalabili/piccole per risparmiare tempo/spazio se vuoi
        # Qui copiamo tutto per sicurezza
        cp -r "/mingw64/share/icons/$theme" "$INSTALL_DIR/share/icons/"
    fi
done

# Loader Pixbuf (per immagini)
mkdir -p "$INSTALL_DIR/lib/gdk-pixbuf-2.0/2.10.0/loaders"
if [ -d "/mingw64/lib/gdk-pixbuf-2.0/2.10.0/loaders" ]; then
    cp /mingw64/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll "$INSTALL_DIR/lib/gdk-pixbuf-2.0/2.10.0/loaders/"
fi
# Genera cache loaders se possibile, altrimenti copia se esiste
if command -v gdk-pixbuf-query-loaders >/dev/null; then
    gdk-pixbuf-query-loaders > "$INSTALL_DIR/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
elif [ -f "/mingw64/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache" ]; then
    cp "/mingw64/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache" "$INSTALL_DIR/lib/gdk-pixbuf-2.0/2.10.0/"
fi

# --- Traduzioni ---
# Copia strutturata per gettext: share/locale/<lang>/LC_MESSAGES/e4maps.mo
echo "Copia traduzioni..."
if [ -d "$BUILD_DIR/po" ]; then
    mkdir -p "$INSTALL_DIR/share/locale"
    for mo_file in "$BUILD_DIR"/po/*.mo; do
        if [ -f "$mo_file" ]; then
            lang=$(basename "$mo_file" .mo)
            mkdir -p "$INSTALL_DIR/share/locale/$lang/LC_MESSAGES"
            cp "$mo_file" "$INSTALL_DIR/share/locale/$lang/LC_MESSAGES/e4maps.mo"
            echo "   -> Lingua: $lang"
        fi
    done
fi

# --- Documentazione e Licenza ---
echo "Copia documentazione..."
if [ -d "docs" ]; then
    mkdir -p "$INSTALL_DIR/share/doc/e4maps"
    cp -r "docs/"* "$INSTALL_DIR/share/doc/e4maps/"
fi
cp LICENSE "$INSTALL_DIR/" 2>/dev/null || true
cp README.md "$INSTALL_DIR/" 2>/dev/null || true

# --- Risorse App ---
if [ -f "e4maps.svg" ]; then
    cp "e4maps.svg" "$INSTALL_DIR/"
fi

# Creazione script di avvio .bat (opzionale ma utile per debug)
cat > "$INSTALL_DIR/run.bat" <<EOF
@echo off
set PATH=%~dp0;%PATH%
start e4maps.exe
EOF

echo
echo "Distribuzione Windows completata in: $INSTALL_DIR"