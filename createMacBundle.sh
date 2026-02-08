#!/bin/bash
#
# Script per creare un bundle .app per macOS e un installer .dmg.
# Utilizza una firma "ad-hoc" che non richiede un Apple Developer ID.
#

# --- IMPOSTAZIONI ---
# Interrompi lo script se un comando fallisce
set -e

APP_NAME="e4maps"
# Il percorso completo del tuo eseguibile compilato (CORREGGI SE DIVERSO)
SOURCE_EXEC="./buildMac/e4maps"
OUTPUT_DIR="./distMac"
# Nome interno del binario (per evitare conflitti con lo script launcher)
REAL_EXEC_NAME="${APP_NAME}_bin"
# Percorso base installazione librerie Homebrew (rilevato automaticamente)
BREW_PREFIX=$(brew --prefix)
# --------------------

echo "📦 Inizio creazione pacchetto per $APP_NAME..."

# 1. Creazione Struttura Directory .app
APP_BUNDLE="$OUTPUT_DIR/$APP_NAME.app"
CONTENTS="$APP_BUNDLE/Contents"
MACOS="$CONTENTS/MacOS"
RESOURCES="$CONTENTS/Resources"
FRAMEWORKS="$CONTENTS/Frameworks" # Convenzione Apple per le librerie

# Pulisci build precedenti e ricrea
rm -rf "$APP_BUNDLE"
mkdir -p "$MACOS"
mkdir -p "$RESOURCES"
mkdir -p "$FRAMEWORKS"

echo "📂 Struttura creata in $APP_BUNDLE"

# 2. Copia dell'eseguibile reale
echo "📁 Copia dell'eseguibile sorgente..."
if [ ! -f "$SOURCE_EXEC" ]; then
    echo "❌ Errore: Eseguibile sorgente non trovato in '$SOURCE_EXEC'. Assicurati di aver compilato il progetto. Uscita."
    exit 1
fi
cp "$SOURCE_EXEC" "$MACOS/$REAL_EXEC_NAME"
chmod +x "$MACOS/$REAL_EXEC_NAME"


# --- 3. Risoluzione Dipendenze Librerie ---
EXEC_PATH="$MACOS/$REAL_EXEC_NAME"
LIBS_DIR="$FRAMEWORKS"

echo "🔗 Inizio analisi e copia delle librerie da Homebrew ($BREW_PREFIX)..."

# Funzione per ottenere le dipendenze Homebrew di un file.
# Cerca qualsiasi libreria che si trovi nel percorso di Homebrew.
get_homebrew_deps() {
    otool -L "$1" | grep "$BREW_PREFIX" | awk '{print $1}'
}

# Array per tenere traccia delle librerie già processate ed evitare duplicati/loop
processed_libs=()

# Funzione ricorsiva per copiare e correggere i percorsi delle dipendenze.
process_dependencies() {
    local target_file="$1"
    
    local dependencies
    dependencies=$(get_homebrew_deps "$target_file")

    for dep_path in $dependencies;
    do
        local lib_name
        lib_name=$(basename "$dep_path")
        local new_lib_path="$LIBS_DIR/$lib_name"
        # Il nuovo percorso che la libreria/eseguibile userà per trovare la dipendenza
        local new_install_name="@executable_path/../Frameworks/$lib_name"

        # Cambia il riferimento nel file target (eseguibile o un'altra libreria)
        # in modo che punti alla copia all'interno del bundle.
        install_name_tool -change "$dep_path" "$new_install_name" "$target_file"

        # Se non abbiamo già processato questa libreria...
        if [[ ! " ${processed_libs[@]} " =~ " ${lib_name} " ]]
        then
            echo "   -> Copiando e analizzando: $lib_name"
            
            # Copia la libreria nel bundle
            cp "$dep_path" "$new_lib_path"
            chmod 644 "$new_lib_path" # Permessi standard per librerie

            # Riscrivi l'ID della libreria appena copiata in modo che si "identifichi"
            # con il suo nuovo percorso all'interno del bundle.
            install_name_tool -id "$new_install_name" "$new_lib_path"
            
            # Aggiungi all'elenco di quelle processate
            processed_libs+=("$lib_name")
            
            # Ricorsione: analizza le dipendenze della libreria appena copiata
            process_dependencies "$new_lib_path"
        fi
    done
}

# Avvia il processo dall'eseguibile principale
process_dependencies "$EXEC_PATH"

echo "✅ Librerie pacchettizzate e percorsi riscritti."

# 4. Gestione Risorse GTK e Altre
echo "🎨 Copia risorse GTK, icone, documenti e traduzioni..."

# Schemi GSettings (essenziali per le impostazioni di GTK)
mkdir -p "$RESOURCES/share/glib-2.0/schemas"
cp "$BREW_PREFIX/share/glib-2.0/schemas/"*.gschema.xml "$RESOURCES/share/glib-2.0/schemas/"
echo "   -> Compilazione schemi GSettings..."
glib-compile-schemas "$RESOURCES/share/glib-2.0/schemas/"

# Temi di icone (Adwaita, hicolor)
mkdir -p "$RESOURCES/share/icons"
if [ -d "$BREW_PREFIX/share/icons/hicolor" ]; then
    cp -r "$BREW_PREFIX/share/icons/hicolor" "$RESOURCES/share/icons/"
fi
if [ -d "$BREW_PREFIX/share/icons/Adwaita" ]; then
    cp -r "$BREW_PREFIX/share/icons/Adwaita" "$RESOURCES/share/icons/"
fi

# Cache dei loader per GdkPixbuf (per caricare formati immagine come PNG, JPG)
mkdir -p "$RESOURCES/lib/gdk-pixbuf-2.0/2.10.0/loaders"
cp -L "$BREW_PREFIX"/lib/gdk-pixbuf-2.0/2.10.0/loaders/*.so "$RESOURCES/lib/gdk-pixbuf-2.0/2.10.0/loaders"
echo "   -> Creazione cache per GdkPixbuf..."
gdk-pixbuf-query-loaders > "$RESOURCES/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"

# Documentazione
if [ -d "./docs" ]; then
    echo "   -> Copia documentazione..."
    mkdir -p "$RESOURCES/share/doc/e4maps"
    cp -r ./docs/* "$RESOURCES/share/doc/e4maps/"
fi

# Traduzioni (.mo)
if [ -d "./buildMac/po" ]; then
    echo "   -> Copia traduzioni..."
    mkdir -p "$RESOURCES/share/locale"
    for lang_path in ./buildMac/po/*.mo;
    do
        if [ -f "$lang_path" ]; then
            lang_code=$(basename "$lang_path" .mo)
            echo "      -> Lingua: $lang_code"
            mkdir -p "$RESOURCES/share/locale/$lang_code/LC_MESSAGES"
            cp "$lang_path" "$RESOURCES/share/locale/$lang_code/LC_MESSAGES/$APP_NAME.mo"
        fi
    done
fi

# Icona dell'applicazione (.icns)
echo "🖼️  Creazione icona applicazione (.icns)..."
ICONSET_DIR="e4maps.iconset"
ICON_FILE=""
if [ -d "./icons" ] && command -v iconutil &> /dev/null; then
    rm -rf "$ICONSET_DIR"
    mkdir -p "$ICONSET_DIR"
    # Crea l'iconset con varie dimensioni
    sips -z 16 16     ./icons/e4maps-16.png  --out "$ICONSET_DIR/icon_16x16.png"
    sips -z 32 32     ./icons/e4maps-32.png  --out "$ICONSET_DIR/icon_16x16@2x.png"
    sips -z 32 32     ./icons/e4maps-32.png  --out "$ICONSET_DIR/icon_32x32.png"
    sips -z 64 64     ./icons/e4maps-64.png  --out "$ICONSET_DIR/icon_32x32@2x.png"
    sips -z 128 128   ./icons/e4maps-128.png --out "$ICONSET_DIR/icon_128x128.png"
    sips -z 256 256   ./icons/e4maps-256.png --out "$ICONSET_DIR/icon_128x128@2x.png"
    sips -z 256 256   ./icons/e4maps-256.png --out "$ICONSET_DIR/icon_256x256.png"
    sips -z 512 512   ./icons/e4maps-512.png --out "$ICONSET_DIR/icon_256x256@2x.png"
    sips -z 512 512   ./icons/e4maps-512.png --out "$ICONSET_DIR/icon_512x512.png"
    sips -z 1024 1024 ./icons/e4maps-512.png --out "$ICONSET_DIR/icon_512x512@2x.png" # `sips` si ferma alla dimensione massima dell'immagine sorgente

    # Converte l'iconset in un file .icns
    iconutil -c icns "$ICONSET_DIR" -o "$RESOURCES/e4maps.icns"
    ICON_FILE="e4maps.icns"
    rm -rf "$ICONSET_DIR"
else
    echo "   -> Attenzione: `iconutil` non trovato o cartella 'icons' assente. L'icona non sarà creata."
fi

# 5. Creazione del Launcher Script
# Questo script imposta le variabili d'ambiente necessarie a GTK per trovare
# le sue risorse (icone, schemi, etc.) all'interno del bundle.
echo "🚀 Creazione dello script di avvio..."
LAUNCHER="$MACOS/$APP_NAME"
cat > "$LAUNCHER" <<EOF
#!/bin/sh
# Questo è il punto di ingresso dell'app.
# Imposta le variabili d'ambiente e poi lancia l'eseguibile reale.

# Trova la directory radice del bundle (.app)
DIR=\$(cd "\$(dirname "\$0")" && pwd)
CONTENTS_DIR="\$(cd "\$DIR/.." && pwd)"
RESOURCES_DIR="\$CONTENTS_DIR/Resources"

# Esporta le variabili d'ambiente per GTK
# Dice a GTK dove trovare i dati (icone, temi)
export XDG_DATA_DIRS="\$RESOURCES_DIR/share:\$XDG_DATA_DIRS"
# Prefisso generale per le risorse GTK
export GTK_DATA_PREFIX="\$RESOURCES_DIR"
# Dove trovare gli schemi delle impostazioni (GSettings)
export GSETTINGS_SCHEMA_DIR="\$RESOURCES_DIR/share/glib-2.0/schemas"
# Dove trovare la cache dei loader per le immagini (PNG, JPG, etc.)
export GDK_PIXBUF_MODULE_FILE="\$RESOURCES_DIR/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"

# Esegui il binario reale, passando tutti gli argomenti
exec "\$DIR/$REAL_EXEC_NAME" "\$@"
EOF
chmod +x "$LAUNCHER"

# 6. Creazione Info.plist
echo "📝 Creazione del file Info.plist..."
cat > "$CONTENTS/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleExecutable</key>
	<string>$APP_NAME</string>
	<key>CFBundleIdentifier</key>
	<string>org.e4maps.e4maps</string>
	<key>CFBundleName</key>
	<string>$APP_NAME</string>
	<key>CFBundlePackageType</key>
	<string>APPL</string>
	<key>CFBundleShortVersionString</key>
	<string>1.0</string>
	<key>CFBundleVersion</key>
	<string>1</string>
	<key>CFBundleIconFile</key>
	<string>$ICON_FILE</string>
	<key>LSMinimumSystemVersion</key>
	<string>11.0</string> 
	<key>NSHighResolutionCapable</key>
	<true/>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
    <key>NSRequiresAquaSystemAppearance</key>
    <false/>
</dict>
</plist>
EOF

# --- 7. Firma Digitale Ad-Hoc ---
# Poiché non hai un ID sviluppatore, usiamo una firma "ad-hoc".
# Questo è sufficiente per eseguire l'app localmente o su altri Mac,
# ma richiederà all'utente di bypassare Gatekeeper la prima volta
# (es. con Ctrl+Click -> Apri).
echo "✍️  Inizio firma digitale (Ad-Hoc)..."
OPTS="--force --options=runtime"

# 1. Firma tutte le librerie e i binari che abbiamo copiato.
echo "   -> Firmo le librerie in Contents/Frameworks..."
if [ -d "$FRAMEWORKS" ]; then
    for lib in "$FRAMEWORKS"/*;
    do
        codesign --sign - $OPTS "$lib"
    done
fi

# 2. Firma l'eseguibile principale
echo "   -> Firmo l'eseguibile principale..."
codesign --sign - $OPTS "$EXEC_PATH"

# 3. Firma il bundle dell'applicazione
echo "   -> Firmo il bundle .app..."
codesign --sign - $OPTS --deep "$APP_BUNDLE"

echo "✅ Firma Ad-Hoc completata."

echo "🎉 Fatto! L'app si trova in: $APP_BUNDLE"
echo "‼️  IMPORTANTE: per avviare l'app su un altro Mac, il proprietario dovrà fare Ctrl+Click sull'icona e scegliere 'Apri'."

# --- 8. Creazione del pacchetto DMG ---
DMG_NAME="$APP_NAME-Installer.dmg"
DMG_PATH="$OUTPUT_DIR/$DMG_NAME"
rm -f "$DMG_PATH"

echo "💿 Creazione del DMG..."
# Prova a usare 'create-dmg' se è installato (brew install create-dmg) per un DMG più carino
if command -v create-dmg &> /dev/null; then
    echo "   -> Uso 'create-dmg' per un installer più gradevole."
    create-dmg \
      --volname "$APP_NAME Installer" \
      --window-pos 200 120 \
      --window-size 800 400 \
      --icon-size 100 \
      --icon "$APP_NAME.app" 200 190 \
      --hide-extension "$APP_NAME.app" \
      --app-drop-link 600 185 \
      "$DMG_PATH" \
      "$APP_BUNDLE"
else
    # Altrimenti, usa il comando base hdiutil
    echo "   -> 'create-dmg' non trovato. Uso 'hdiutil' per un DMG standard."
    hdiutil create -volname "$APP_NAME" -srcfolder "$APP_BUNDLE" -ov -format UDZO "$DMG_PATH"
fi

echo ""
echo "🚀 TUTTO COMPLETATO! Il tuo installer è pronto qui: $DMG_PATH"