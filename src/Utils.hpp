#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <algorithm>
#include <vector>
#include <gtkmm.h> // Added for Gtk::Window

// Namespace for utility functions
namespace Utils {
    bool isValidImageFile(const std::string& path);
    void hexToCairo(const std::string& hex, double& r, double& g, double& b, double& a);
    std::string cairoToHex(double r, double g, double b, double a = 1.0);
    void openInBrowser(Gtk::Window& parent, const std::string& url);
}

#endif // UTILS_HPP