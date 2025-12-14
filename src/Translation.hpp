#ifndef TRANSLATION_HPP
#define TRANSLATION_HPP

#include <libintl.h>
#include <locale.h>
#include <string>
#include <cstdlib>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <sys/syslimits.h> // For PATH_MAX
#endif

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

#ifdef __APPLE__
    // On macOS, GUI apps might not inherit the shell's locale environment variables.
    CFArrayRef languages = CFLocaleCopyPreferredLanguages();
    if (languages) {
        if (CFArrayGetCount(languages) > 0) {
            CFStringRef language = (CFStringRef)CFArrayGetValueAtIndex(languages, 0);
            char langBuf[64];
            if (CFStringGetCString(language, langBuf, sizeof(langBuf), kCFStringEncodingUTF8)) {
                std::string langStr(langBuf);
                // Convert typical macOS locale (e.g., "en-US") to POSIX format (e.g., "en_US")
                for (char &c : langStr) {
                    if (c == '-') c = '_';
                }
                // Ensure UTF-8
                if (langStr.find('.') == std::string::npos) {
                     langStr += ".UTF-8";
                }
                setenv("LANG", langStr.c_str(), 1);
            }
        }
        CFRelease(languages);
    }

    // Locate the locale directory inside the application bundle
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    if (mainBundle) {
        CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
        if (resourcesURL) {
            char path[PATH_MAX];
            if (CFURLGetFileSystemRepresentation(resourcesURL, true, (UInt8*)path, PATH_MAX)) {
                std::string resourcesPath(path);
                // The script puts translations in Resources/share/locale
                localeDir = resourcesPath + "/share/locale";
            }
            CFRelease(resourcesURL);
        }
    }
#endif
    
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