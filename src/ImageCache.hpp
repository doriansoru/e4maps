#ifndef IMAGE_CACHE_HPP
#define IMAGE_CACHE_HPP

#include <gtkmm.h>
#include <string>
#include <map>
#include <tuple>

// Image cache management class
class ImageCache {
private:
    std::map<std::tuple<std::string, int, int>, Glib::RefPtr<Gdk::Pixbuf>> cache;

    ImageCache() = default;

public:
    ImageCache(const ImageCache&) = delete;
    ImageCache& operator=(const ImageCache&) = delete;

    Glib::RefPtr<Gdk::Pixbuf> getCachedImage(const std::string& path, int reqW, int reqH);
    void clear();

    static ImageCache& getInstance();
};

#endif // IMAGE_CACHE_HPP
