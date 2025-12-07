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

#endif // UTILS_HPP