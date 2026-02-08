#ifndef EXPORTMANAGER_HPP
#define EXPORTMANAGER_HPP

#include <string>
#include <memory>
#include <functional>
#include <gtkmm.h>
#include "MindMap.hpp"
#include "Exporter.hpp"

class ExportManager {
public:
    ExportManager(Gtk::Window& parentWindow);

    void exportToPng(std::shared_ptr<MindMap> map);
    void exportToPdf(std::shared_ptr<MindMap> map);
    void exportToFreeplane(std::shared_ptr<MindMap> map);

    // Set callback for status updates
    void setStatusCallback(std::function<void(const std::string&)> cb);

private:
    Gtk::Window& m_parentWindow;
    std::function<void(const std::string&)> m_statusCallback;

    // Helper for file dialog and execution
    void handleExport(const std::string& format, 
                      const std::string& default_filename,
                      std::shared_ptr<MindMap> map,
                      std::function<void(Exporter&, std::shared_ptr<MindMap>, const std::string&, double)> render_func,
                      double dpi = 0.0);
    
    // Helper to get last used directory (could be injected via config manager in future)
    std::string getLastUsedDirectory();
    void updateLastUsedDirectory(const std::string& path);
};

#endif // EXPORTMANAGER_HPP
