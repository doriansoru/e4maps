#ifndef TRANSLATION_HPP
#define TRANSLATION_HPP

#include <libintl.h>
#include <locale.h>
#include <string>
#ifdef _WIN32
#include <windows.h>
#include <vector>
#endif

// Define translation macros
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop(String)

// Initialize the translation system
inline void init_translation(const std::string& domain, const std::string& directory) {
    std::string localeDir = directory;
    
#ifdef _WIN32
    // On Windows, determine the locale directory relative to the executable
    std::vector<char> path(MAX_PATH);
    if (GetModuleFileNameA(NULL, path.data(), path.size())) {
        std::string exePath(path.data());
        std::string::size_type pos = exePath.find_last_of("\\/");
        if (pos != std::string::npos) {
            // Assume standard Linux-like layout on Windows:
            // bin/executable.exe
            // share/locale
            localeDir = exePath.substr(0, pos) + "\\..\\share\\locale";
        }
    }
#endif

    setlocale(LC_ALL, "");
    bindtextdomain(domain.c_str(), localeDir.c_str());
    textdomain(domain.c_str());
}

// Initialize translation with default values
inline void init_translation() {
    init_translation("e4maps", "/usr/share/locale");
}

#endif // TRANSLATION_HPP