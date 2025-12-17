#include "Utils.hpp"
#include <stdexcept>
#include <string>
#include <iostream>
#include <cstdlib> // For system()
#include <gtkmm.h> // Required for Gtk::show_uri_on_window

namespace Utils {
    bool isValidImageFile(const std::string& path) {
        if (path.empty()) return false;

        static const std::vector<std::string> validExtensions = {".png", ".jpg", ".jpeg", ".gif", ".bmp"};
        std::string lowerPath = path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

        for (const auto& ext : validExtensions) {
            if (lowerPath.length() >= ext.length() &&
                lowerPath.substr(lowerPath.length() - ext.length()) == ext) {
                return true;
            }
        }
        return false;
    }

    void hexToCairo(const std::string& hex, double& r, double& g, double& b, double& a) {
        if (hex.empty() || hex[0] != '#') {
            r = g = b = 0.0; a = 1.0; return; // Default to black
        }

        unsigned int ir, ig, ib, ia = 255;
        if (hex.length() == 9) { // #RRGGBBAA
            sscanf(hex.c_str(), "#%02x%02x%02x%02x", &ir, &ig, &ib, &ia);
        } else { // #RRGGBB
            sscanf(hex.c_str(), "#%02x%02x%02x", &ir, &ig, &ib);
        }
        r = ir / 255.0;
        g = ig / 255.0;
        b = ib / 255.0;
        a = ia / 255.0;
    }

    std::string cairoToHex(double r, double g, double b, double a) {
        char buffer[10];
        if (a >= 0.999) { // Omit alpha if fully opaque
            snprintf(buffer, sizeof(buffer), "#%02X%02X%02X",
                     (int)(r * 255), (int)(g * 255), (int)(b * 255));
        } else {
            snprintf(buffer, sizeof(buffer), "#%02X%02X%02X%02X",
                     (int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(a * 255));
        }
        return std::string(buffer);
    }

    void openInBrowser(Gtk::Window& /*parent*/, const std::string& url) {
        // Fallback to system command
        #ifdef _WIN32
            std::string command = "start \"" + url + "\"";
        #elif __APPLE__
            std::string command = "open \"" + url + "\"";
        #else
            std::string command = "xdg-open \"" + url + "\"";
        #endif
        
        if (system(command.c_str()) != 0) {
            std::cerr << "Failed to open URL: " << url << std::endl;
        }
    }
}