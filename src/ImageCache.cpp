#include "ImageCache.hpp"
#include "Utils.hpp"
#include <iostream>
#include <algorithm>

Glib::RefPtr<Gdk::Pixbuf> ImageCache::getCachedImage(const std::string& path, int reqW, int reqH) {
    if (path.empty()) return {};

    // Validate the file is a supported image type
    if (!Utils::isValidImageFile(path)) {
        std::cerr << "Warning: Attempting to load unsupported image file: " << path << std::endl;
        return {};
    }

    auto key = std::make_tuple(path, reqW, reqH);
    if (cache.count(key)) return cache[key];

    try {
        auto raw = Gdk::Pixbuf::create_from_file(path);
        if (!raw) {
            std::cerr << "Error: Could not load image file: " << path << std::endl;
            return {};
        }

        int w = raw->get_width();
        int h = raw->get_height();
        double original_ratio = (double)w / h;

        int targetW = w;
        int targetH = h;
        Glib::RefPtr<Gdk::Pixbuf> scaled_pixbuf;

        if (reqW > 0 && reqH == 0) { // Fixed width, auto height
                targetW = reqW;
                targetH = static_cast<int>(reqW / original_ratio);
        } else if (reqH > 0 && reqW == 0) { // Fixed height, auto width
                targetH = reqH;
                targetW = static_cast<int>(reqH * original_ratio);
        } else if (reqW > 0 && reqH > 0) { // Both fixed: fit inside box
            double scale_factor = std::min((double)reqW / w, (double)reqH / h);
            targetW = static_cast<int>(w * scale_factor);
            targetH = static_cast<int>(h * scale_factor);
        } else { // reqW == 0 && reqH == 0 (auto-scale to default context size)
            int maxDim = 150; // Default max size for nodes if not specified
            if (w > maxDim || h > maxDim) {
                double s = (double)maxDim / std::max(w, h);
                targetW = static_cast<int>(w * s);
                targetH = static_cast<int>(h * s);
                }
        }

        if (targetW == w && targetH == h) {
            scaled_pixbuf = raw;
        } else {
            scaled_pixbuf = raw->scale_simple(targetW, targetH, Gdk::INTERP_BILINEAR);
        }

        cache[key] = scaled_pixbuf;
        return scaled_pixbuf;
    } catch(const Glib::Exception& e) {
        std::cerr << "Error loading image file '" << path << "': " << e.what() << std::endl;
        return {};
    } catch(const std::exception& e) {
        std::cerr << "Error loading image file '" << path << "': " << e.what() << std::endl;
        return {};
    } catch(...) {
        std::cerr << "Error loading image file '" << path << "'" << std::endl;
        return {};
    }
}

void ImageCache::clear() {
    cache.clear();
}

ImageCache& ImageCache::getInstance() {
    static ImageCache instance;
    return instance;
}
