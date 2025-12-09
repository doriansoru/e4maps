#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <algorithm>
#include <vector>

// Global utility function for XML escaping to avoid code duplication
inline std::string escapeXml(const std::string& data) {
    std::string buffer;
    buffer.reserve(data.size());
    for(size_t pos = 0; pos != data.size(); ++pos) {
        switch(data[pos]) {
            case '&':  buffer.append("&amp;");       break;
            case '"': buffer.append("&quot;");      break;
            case '\'': buffer.append("&apos;");      break;
            case '<':  buffer.append("&lt;");        break;
            case '>':  buffer.append("&gt;");        break;
            default:   buffer.append(&data[pos], 1); break;
        }
    }
    return buffer;
}

// Helper function to validate image files to avoid code duplication
inline bool isValidImageFile(const std::string& path) {
    if (path.empty()) return false;

    std::vector<std::string> validExtensions = {".png", ".jpg", ".jpeg", ".gif", ".bmp"};
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

// Convert hex string (#RRGGBB or #RRGGBBAA) to RGBA values (0.0 - 1.0)
inline void hexToCairo(const std::string& hex, double& r, double& g, double& b, double& a) {
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

// Convert RGBA values to hex string (#RRGGBBAA)
inline std::string cairoToHex(double r, double g, double b, double a = 1.0) {
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


#endif // UTILS_HPP