#!/bin/bash

# Script DEFINITIVO per la distribuzione di e4maps su Windows
# Risolve i problemi di caricamento SVG e dipendenze ricorsive module-loading

set -e

APP_NAME="e4maps"

# 1. Rilevamento directory di build
if [ -d "./build" ] && [ -f "./build/$APP_NAME.exe" ]; then
    BUILD_DIR="./build"
elif [ -d "./buildWin" ] && [ -f "./buildWin/$APP_NAME.exe" ]; then
    BUILD_DIR="./buildWin"
else
    echo "ERRORE: Eseguibile non trovato."
    exit 1
fi

INSTALL_DIR="./distWin"
rm -rf "$INSTALL_DIR"
mkdir -p "$INSTALL_DIR"

echo "📦 Confezionamento professionale per Windows..."

# 2. Copia eseguibile
cp "$BUILD_DIR/$APP_NAME.exe" "$INSTALL_DIR/"

# 3. Risorse GTK e Loader (Copia subito per scansionarne le dipendenze dopo)
echo "🎨 Preparazione risorse e moduli..."

# Loader GdkPixbuf (essenziali per SVG, PNG, etc.)
LOADERS_DIR="lib/gdk-pixbuf-2.0/2.10.0/loaders"
mkdir -p "$INSTALL_DIR/$LOADERS_DIR"
cp /mingw64/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.dll "$INSTALL_DIR/$LOADERS_DIR/"

# Schemi GSettings
mkdir -p "$INSTALL_DIR/share/glib-2.0/schemas"
cp /mingw64/share/glib-2.0/schemas/*.xml "$INSTALL_DIR/share/glib-2.0/schemas/"
glib-compile-schemas "$INSTALL_DIR/share/glib-2.0/schemas/"

# Icone Adwaita (copiamo tutto il necessario per evitare 'image-missing')
mkdir -p "$INSTALL_DIR/share/icons"
cp -r /mingw64/share/icons/Adwaita "$INSTALL_DIR/share/icons/"
cp -r /mingw64/share/icons/hicolor "$INSTALL_DIR/share/icons/"

# 4. Raccolta TOTALE e RICORSIVA delle DLL
echo "🔗 Analisi profonda delle dipendenze..."
# Troviamo tutte le DLL necessarie scansionando ricorsivamente tutto (eseguibile + loader)
collect_all_deps() {
    local target_dir="$1"
    local found_new=true
    
    while [ "$found_new" = true ]; do
        found_new=false
        # Trova tutti i binari correnti (exe e dll in ogni sottocartella)
        local binaries=$(find "$target_dir" -name "*.exe" -o -name "*.dll")
        
        # Scansiona le loro dipendenze
        local all_deps=$(ldd $binaries 2>/dev/null | grep -o '/mingw64/bin/[^ ]*\.dll' | sort | uniq)
        
        for dep in $all_deps; do
            local name=$(basename "$dep")
            # Se la DLL non è ancora nella root dell'installazione, copiala
            if [ ! -f "$target_dir/$name" ]; then
                cp "$dep" "$target_dir/"
                found_new=true
                # echo "   + $name"
            fi
        done
    done
}

collect_all_deps "$INSTALL_DIR"

# 5. Generazione Cache Loader Pixbuf
# Questo passaggio è vitale: dice a GTK dove trovare i file .dll per caricare le immagini
echo "⚙️  Generazione cache moduli grafici..."
# Generiamo la query. I percorsi nel file devono essere relativi alla root dell'app
gdk-pixbuf-query-loaders > "$INSTALL_DIR/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
# Convertiamo i percorsi assoluti di MSYS2 in percorsi relativi per la portable app
# Esempio: "C:/msys64/mingw64/lib/..." -> "lib/..."
sed -i 's|.*\/lib/gdk-pixbuf-2.0/|lib/gdk-pixbuf-2.0/|g' "$INSTALL_DIR/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"

# 6. Traduzioni
echo "🌐 Configurazione lingue..."
if [ -d "$BUILD_DIR/po" ]; then
    for mo_file in "$BUILD_DIR"/po/*.mo; do
        if [ -f "$mo_file" ]; then
            lang=$(basename "$mo_file" .mo)
            dest="$INSTALL_DIR/share/locale/$lang/LC_MESSAGES"
            mkdir -p "$dest"
            cp "$mo_file" "$dest/e4maps.mo"
        fi
    done
fi

# 7. File extra
cp LICENSE README.md "$INSTALL_DIR/" 2>/dev/null || true

# 8. Script di avvio per forzare il caricamento moduli
cat > "$INSTALL_DIR/run_e4maps.bat" <<EOF
@echo off
set GDK_PIXBUF_MODULE_FILE=%~dp0lib\gdk-pixbuf-2.0\2.10.0\loaders.cache
start e4maps.exe
EOF

echo
echo "✅ Distribuzione completata con successo in: $INSTALL_DIR"
echo "Usa 'run_e4maps.bat' per avviare l'applicazione con il supporto immagini garantito."