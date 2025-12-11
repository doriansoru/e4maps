#include "Utils.hpp"
#include <cstdlib>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#endif

namespace Utils {
    std::string escapeXml(const std::string& data) {
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

    bool isValidImageFile(const std::string& path) {
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

    void openInBrowser(const std::string& url) {
#ifdef _WIN32
        ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#elif __APPLE__
        std::string command = "open \"" + url + "\"";
        system(command.c_str());
#else
        std::string command = "xdg-open \"" + url + "\"";
        int result = system(command.c_str());
        if (result == -1) {
            // Fallback to x-www-browser if xdg-open is not available
            command = "x-www-browser \"" + url + "\"";
            system(command.c_str());
        }
#endif
    }
}