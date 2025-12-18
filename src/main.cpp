#include <gtkmm/application.h>
#include "MainWindow.hpp"
#include "Translation.hpp"
#include <vector>
#include <string>
#include <iostream>

int main(int argc, char *argv[]) {
    // Initialize translation system
    init_translation("e4maps", LOCALEDIR);

    // Parse command line arguments to find a file to open
    std::string fileToOpen;
    std::vector<char*> newArgv;
    
    // Always keep the program name (argv[0])
    if (argc > 0) {
        newArgv.push_back(argv[0]);
    }

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            std::cout << _("Usage: ") << argv[0] << " [OPTIONS] [FILE]\n";
            std::cout << _("  FILE       Optional path to a .e4m file to open on startup.\n\n");
        }

        // Simple heuristic: if it doesn't start with '-', treat it as the filename
        // Only accept the first such argument as the file to open
        if (fileToOpen.empty() && argv[i][0] != '-') {
            fileToOpen = argv[i];
        } else {
            newArgv.push_back(argv[i]);
        }
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
