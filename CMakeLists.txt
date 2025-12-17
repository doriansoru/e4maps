cmake_minimum_required(VERSION 3.14)
project(e4maps VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Trova il pacchetto Cairo e GTKMM
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(CAIROMM REQUIRED cairomm-1.0)
# Optional WebKit2GTK for Linux, but we're switching to external browser
# pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)

# Include tinyxml2 from 3rdparty
set(TINYXML2_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/tinyxml2)
include_directories(${TINYXML2_SOURCE_DIR})

# Find Gettext for internationalization support
find_package(Gettext REQUIRED)

# Find the intl library (usually part of libintl)
find_library(INTL_LIBRARY NAMES intl)
find_path(INTL_INCLUDE_DIR libintl.h)

if(INTL_LIBRARY AND INTL_INCLUDE_DIR)
    set(Intl_FOUND TRUE)
    set(Intl_LIBRARIES ${INTL_LIBRARY})
    set(Intl_INCLUDE_DIRS ${INTL_INCLUDE_DIR})
endif()

# Include internationalization settings
if(Intl_FOUND)
    include_directories(${Intl_INCLUDE_DIRS})
    link_directories(${Intl_LIBRARY_DIRS})
    list(APPEND EXTRA_LIBS ${Intl_LIBRARIES})
endif()

include_directories(${GTKMM_INCLUDE_DIRS} ${CAIROMM_INCLUDE_DIRS})
link_directories(${GTKMM_LIBRARY_DIRS} ${CAIROMM_LIBRARY_DIRS})

# Define the locale directory for translations
add_definitions(-DLOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale")
add_definitions(-DDOCDIR="${CMAKE_INSTALL_PREFIX}/share/doc/${PROJECT_NAME}")
add_definitions(-DGETTEXT_PACKAGE="e4maps")

# Add source files
# Check if we're on Windows
if(WIN32)
    # Add Windows resource file for icon
    configure_file(resources/e4maps.ico resources/e4maps.ico COPYONLY)
    add_executable(e4maps WIN32
        src/main.cpp
        src/Translation.hpp
        src/MapArea.cpp
        src/MainWindow.cpp
        src/MainWindow_UI.cpp
        src/MainWindow_Actions.cpp
        src/NodeEditDialog.cpp
        src/LayoutAlgorithm.cpp
        src/Theme.cpp
        src/ThemeEditor.cpp
        src/Utils.cpp
        src/MindMap.cpp
        ${TINYXML2_SOURCE_DIR}/tinyxml2.cpp
        resources/e4maps.rc
    )
    add_definitions(-DAPP_NAME_STR="\\\"${PROJECT_NAME}\\\"")
else()
    add_executable(e4maps
        src/main.cpp
        src/Translation.hpp
        src/MapArea.cpp
        src/MainWindow.cpp
        src/MainWindow_UI.cpp
        src/MainWindow_Actions.cpp
        src/NodeEditDialog.cpp
        src/LayoutAlgorithm.cpp
        src/Theme.cpp
        src/ThemeEditor.cpp
        src/Utils.cpp
        src/MindMap.cpp
        ${TINYXML2_SOURCE_DIR}/tinyxml2.cpp
    )
endif()

# Add header files to the executable target to ensure they are included in IDEs
set(HEADER_FILES
    src/MainWindow.hpp
    src/NodeEditDialog.hpp
    src/Utils.hpp
    src/Exporter.hpp
    src/MindMap.hpp
    src/MindMapDrawer.hpp
    src/DrawingContext.hpp
    src/Command.hpp
    src/MapArea.hpp
    src/Constants.hpp
    src/ConfigManager.hpp
    src/LayoutAlgorithm.hpp
    src/MindMapUtils.hpp
    src/Theme.hpp
    src/ThemeEditor.hpp
)

target_sources(e4maps PRIVATE ${HEADER_FILES})

target_link_libraries(e4maps PRIVATE ${GTKMM_LIBRARIES} ${CAIROMM_LIBRARIES} ${EXTRA_LIBS} m)

if(APPLE)
    find_library(CORE_FOUNDATION_FRAMEWORK CoreFoundation)
    if(CORE_FOUNDATION_FRAMEWORK)
        target_link_libraries(e4maps PRIVATE ${CORE_FOUNDATION_FRAMEWORK})
    endif()
endif()

# i18n support
if(GETTEXT_FOUND)
    # Define POT file and translation template
    set(POT_FILE "${CMAKE_CURRENT_BINARY_DIR}/e4maps.pot")

    # Create pot file from source code
    add_custom_target(update-translations
        COMMAND xgettext --package-name="e4maps" --package-version="1.0.0"
	--copyright-holder="Dorian Soru" --msgid-bugs-address="doriansoru@gmail.com"
            --keyword=_ --keyword=N_ --from-code=UTF-8 -o ${POT_FILE}
            ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Updating translation template"
    )

    # Create target to update all translation files using our script
    add_custom_target(update-all-translations
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/update-translations.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Updating all translation files and generating MO files"
        DEPENDS update-translations
    )

    # Create target to compile PO files to MO files without extracting new strings
    add_custom_target(compile-po-to-mo
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/compile-translations.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling PO files to MO files"
    )
endif()

# Install the executable
install(TARGETS e4maps
        RUNTIME DESTINATION bin)

# Install icon and desktop file
install(FILES e4maps.svg
        DESTINATION share/icons/hicolor/scalable/apps/
        RENAME e4maps.svg)

# Install PNG icons in different sizes
install(FILES icons/e4maps-16.png
        DESTINATION share/icons/hicolor/16x16/apps/
        RENAME e4maps.png)

install(FILES icons/e4maps-24.png
        DESTINATION share/icons/hicolor/24x24/apps/
        RENAME e4maps.png)

install(FILES icons/e4maps-32.png
        DESTINATION share/icons/hicolor/32x32/apps/
        RENAME e4maps.png)

install(FILES icons/e4maps-48.png
        DESTINATION share/icons/hicolor/48x48/apps/
        RENAME e4maps.png)

install(FILES icons/e4maps-64.png
        DESTINATION share/icons/hicolor/64x64/apps/
        RENAME e4maps.png)

install(FILES icons/e4maps-128.png
        DESTINATION share/icons/hicolor/128x128/apps/
        RENAME e4maps.png)

install(FILES icons/e4maps-256.png
        DESTINATION share/icons/hicolor/256x256/apps/
        RENAME e4maps.png)

install(FILES icons/e4maps-512.png
        DESTINATION share/icons/hicolor/512x512/apps/
        RENAME e4maps.png)

install(FILES e4maps.desktop
        DESTINATION share/applications/)

# Install documentation
install(FILES docs/user_guide_en.html docs/user_guide_it.html
        DESTINATION share/doc/${PROJECT_NAME})

# Generate MO files from PO files and install them in the correct structure
file(GLOB PO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/po/*.po)

# Create target to generate .mo files from .po files
foreach(PO_FILE ${PO_FILES})
    get_filename_component(LANG ${PO_FILE} NAME_WE)
    set(MO_FILE ${CMAKE_CURRENT_BINARY_DIR}/po/${LANG}/LC_MESSAGES/e4maps.mo)

    add_custom_command(
        OUTPUT ${MO_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/po/${LANG}/LC_MESSAGES
        COMMAND msgfmt ${PO_FILE} -o ${MO_FILE}
        DEPENDS ${PO_FILE}
        COMMENT "Generating ${LANG}.mo from ${PO_FILE}"
    )

    list(APPEND MO_FILES ${MO_FILE})
endforeach()

add_custom_target(compile-translations ALL DEPENDS ${MO_FILES})

# Install the generated .mo files in the correct structure
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/po/
        DESTINATION share/locale/
        FILES_MATCHING PATTERN "*.mo")