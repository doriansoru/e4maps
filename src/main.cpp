#include <gtkmm/application.h>
#include "Gui.hpp"
#include "Translation.hpp"

int main(int argc, char *argv[]) {
    // Initialize translation system
    init_translation("e4maps", LOCALEDIR);

    auto app = Gtk::Application::create(argc, argv, "org.e4maps.app");

    MainWindow window;

    // Avvia il loop eventi
    return app->run(window);
}
