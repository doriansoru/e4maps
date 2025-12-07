#ifndef TRANSLATION_HPP
#define TRANSLATION_HPP

#include <libintl.h>
#include <locale.h>
#include <string>

// Define translation macros
#define _(String) gettext(String)
#define gettext_noop(String) String
#define N_(String) gettext_noop(String)

// Initialize the translation system
inline void init_translation(const std::string& domain, const std::string& directory) {
    setlocale(LC_ALL, "");
    bindtextdomain(domain.c_str(), directory.c_str());
    textdomain(domain.c_str());
}

// Initialize translation with default values
inline void init_translation() {
    init_translation("e4maps", "/usr/share/locale");
}

#endif // TRANSLATION_HPP