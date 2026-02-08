#include <gtkmm/application.h>
#include "MainWindow.hpp"
#include "Translation.hpp"
#include "MapSerializer.hpp"
#include "Exporter.hpp"
#include "LayoutAlgorithm.hpp"
#include "MindMapUtils.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <filesystem>

void show_usage(char* progname) {
    std::cout << _("Usage: ") << progname << " [OPTIONS] [FILE]\n";
    std::cout << _("Options:\n");
    std::cout << _("  -h, --help               Show this help message\n");
    std::cout << _("  --convert-to FORMAT      Convert FILE to FORMAT (pdf, png, mm)\n");
    std::cout << _("  --auto-layout            Automatically space all elements in FILE\n\n");
    std::cout << _("  FILE                     Path to a .e4m file to open or convert.\n");
}

int main(int argc, char *argv[]) {
    // Initialize translation system
    init_translation("e4maps", LOCALEDIR);

    std::string convertToFormat;
    bool autoLayout = false;
    std::string fileToOpen;
    std::vector<char*> newArgv;
    
    // Always keep the program name (argv[0])
    if (argc > 0) {
        newArgv.push_back(argv[0]);
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            show_usage(argv[0]);
            return 0;
        } else if (arg == "--convert-to") {
            if (i + 1 < argc) {
                convertToFormat = argv[++i];
            } else {
                std::cerr << _("Error: --convert-to requires a format (pdf, png, mm)\n");
                return 1;
            }
        } else if (arg == "--auto-layout") {
            autoLayout = true;
        } else if (arg[0] != '-') {
            if (fileToOpen.empty()) {
                fileToOpen = arg;
            } else {
                newArgv.push_back(argv[i]);
            }
        } else {
            newArgv.push_back(argv[i]);
        }
    }
    
    if (autoLayout || !convertToFormat.empty()) {
        if (fileToOpen.empty()) {
            std::cerr << _("Error: No input file specified.\n");
            return 1;
        }

        // Initialize GTK for Pango/Cairo rendering
        auto app = Gtk::Application::create("org.e4maps.cli", Gio::APPLICATION_NON_UNIQUE);

        auto map = MapSerializer::load(fileToOpen);
        if (!map) {
            std::cerr << _("Error: Could not load map file: ") << fileToOpen << std::endl;
            return 1;
        }

        if (autoLayout) {
            std::cout << _("Applying auto-layout...") << std::endl;
            
            // Pre-calculate node dimensions using a temporary Cairo context
            // This is needed for the layout algorithm to account for node sizes
            auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 1, 1);
            auto cr = Cairo::Context::create(surface);
            MindMapDrawer drawer;
            drawer.preCalculateNodeDimensions(map->root, map->theme, cr);

            MindMapUtils::resetManualPositionsRecursive(map->root);
            
            // Apply standard Tree Layout
            LayoutAlgorithms::calculateTreeLayout(map->root);
            
            if (convertToFormat.empty()) {
                MapSerializer::save(map, fileToOpen);
                std::cout << _("Auto-layout applied and saved to ") << fileToOpen << std::endl;
                return 0;
            }
        }

        std::filesystem::path inputPath(fileToOpen);
        std::filesystem::path outputPath = inputPath;
        
        Exporter exporter(800, 600);
        
        if (convertToFormat == "pdf") {
            outputPath.replace_extension(".pdf");
            exporter.exportToPdf(map, outputPath.string());
        } else if (convertToFormat == "png") {
            outputPath.replace_extension(".png");
            exporter.exportToPng(map, outputPath.string(), 300.0); // 300 DPI for high quality CLI export
        } else if (convertToFormat == "mm") {
            outputPath.replace_extension(".mm");
            exporter.exportToFreeplane(map, outputPath.string());
        } else {
            std::cerr << _("Error: Unknown format: ") << convertToFormat << _(". Supported: pdf, png, mm\n");
            return 1;
        }
        
        std::cout << _("Successfully converted ") << fileToOpen << _(" to ") << outputPath.string() << std::endl;
        return 0;
    }

    int newArgc = newArgv.size();
    char** newArgvPtr = newArgv.data();

    auto app = Gtk::Application::create(newArgc, newArgvPtr, "org.e4maps.app");

    MainWindow window;
    
    if (!fileToOpen.empty()) {
        window.openFile(fileToOpen);
    }

    // Avvia il loop eventi
    return app->run(window);
}
