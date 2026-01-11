#include "Utils.hpp"
#include <stdexcept>
#include <string>
#include <iostream>
#include <cstdlib> // For system()
#include <gtkmm.h> // Required for Gtk::show_uri_on_window
#include <pangomm.h>
#include <regex>
#include <set>

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

    void setPangoLayoutText(Glib::RefPtr<Pango::Layout> layout, const std::string& text) {
        GError* error = nullptr;
        char* text_out = nullptr;
        PangoAttrList* attrs = nullptr;
        
        // Pre-process text to convert common HTML tags to Pango markup
        std::string processedText = text;
        
        auto replaceAll = [&](std::string& s, const std::string& from, const std::string& to) {
            if (from.empty()) return;
            size_t start_pos = 0;
            while((start_pos = s.find(from, start_pos)) != std::string::npos) {
                s.replace(start_pos, from.length(), to);
                start_pos += to.length();
            }
        };

        // Convert standard HTML to Pango tags
        replaceAll(processedText, "<strong>", "<b>");
        replaceAll(processedText, "</strong>", "</b>");
        replaceAll(processedText, "<em>", "<i>");
        replaceAll(processedText, "</em>", "</i>");
        replaceAll(processedText, "<br>", "\n");
        replaceAll(processedText, "<br/>", "\n");
        replaceAll(processedText, "<strike>", "<s>");
        replaceAll(processedText, "</strike>", "</s>");
        
        // Check if the text is valid markup (Attempt 1)
        if (pango_parse_markup(processedText.c_str(), -1, 0, &attrs, &text_out, nullptr, &error)) {
            layout->set_markup(processedText);
            g_free(text_out);
            pango_attr_list_unref(attrs);
            return;
        }

        // Attempt 1 Failed.
        // It's likely due to unsupported tags (like <ul>, <li>, <p>) or malformed attributes.
        // Clean up error for next attempt
        if (error) { g_error_free(error); error = nullptr; }

        // Sanitize: Strip unknown tags, keeping supported ones.
        // Pango supported tags: b, big, i, s, sub, sup, small, tt, u, span, a
        static const std::set<std::string> allowedTags = {
            "b", "big", "i", "s", "sub", "sup", "small", "tt", "u", "span", "a"
        };

        std::string sanitizedText = "";
        std::string::const_iterator searchStart(processedText.cbegin());
        
        // Regex to match tags: </?tagName...>
        // Group 1: Closing slash (optional)
        // Group 2: Tag Name
        // Group 3: Attributes and closing bracket
        std::regex tagRegex("<(/?)([a-zA-Z0-9_]+)([^>]*)>");
        std::smatch match;

        while (std::regex_search(searchStart, processedText.cend(), match, tagRegex)) {
            // Append text before the tag
            sanitizedText += match.prefix();
            
            std::string tagName = match[2];
            // Normalize to lowercase for checking
            std::transform(tagName.begin(), tagName.end(), tagName.begin(), ::tolower);

            if (allowedTags.count(tagName)) {
                // It's a supported tag, keep it exactly as is
                sanitizedText += match[0];
            } else {
                // Unsupported tag (e.g. <p>, <div>, <custom>). 
                // Skip it (effectively removing it), but keep the content that follows.
                // If it was <br> it was already converted to \n above.
            }

            searchStart = match.suffix().first;
        }
        sanitizedText += std::string(searchStart, processedText.cend());

        // Check if the sanitized text is valid markup (Attempt 2)
        if (pango_parse_markup(sanitizedText.c_str(), -1, 0, &attrs, &text_out, nullptr, &error)) {
            layout->set_markup(sanitizedText);
            g_free(text_out);
            pango_attr_list_unref(attrs);
        } else {
            // Still invalid (maybe mismatched tags like <b><i></b></i>). 
            // Fallback to displaying raw original text so the user can fix it.
            if (error) { g_error_free(error); }
            layout->set_text(text);
        }
    }
}