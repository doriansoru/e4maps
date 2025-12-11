#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <algorithm>
#include <vector>

// Namespace for utility functions
namespace Utils {
    std::string escapeXml(const std::string& data);
    bool isValidImageFile(const std::string& path);
    void hexToCairo(const std::string& hex, double& r, double& g, double& b, double& a);
    std::string cairoToHex(double r, double g, double b, double a = 1.0);
    void openInBrowser(const std::string& url);
}

#endif // UTILS_HPP