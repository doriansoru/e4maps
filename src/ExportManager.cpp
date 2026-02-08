#include "ExportManager.hpp"
#include "Translation.hpp"
#include "ConfigManager.hpp" 
#include <iostream>
#include <filesystem>

ExportManager::ExportManager(Gtk::Window& parentWindow)
    : m_parentWindow(parentWindow) {}

void ExportManager::setStatusCallback(std::function<void(const std::string&)> cb) {
    m_statusCallback = cb;
}

void ExportManager::exportToPng(std::shared_ptr<MindMap> map) {
    Gtk::Dialog exportDialog(_("Export to PNG"), m_parentWindow, true);
    exportDialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    exportDialog.add_button(_("Export"), Gtk::RESPONSE_OK);
    exportDialog.set_default_response(Gtk::RESPONSE_OK);

    Gtk::VBox vbox;
    vbox.set_spacing(10);
    vbox.set_margin_left(10);
    vbox.set_margin_right(10);
    vbox.set_margin_top(10);
    vbox.set_margin_bottom(10);

    Gtk::Label label(_("Select resolution for PNG export:"));
    vbox.pack_start(label, Gtk::PACK_SHRINK);

    Gtk::RadioButtonGroup resolutionGroup;
    Gtk::RadioButton radio72(resolutionGroup, _("72 DPI (Screen)"));
    Gtk::RadioButton radio300(resolutionGroup, _("300 DPI (High Quality)"));
    Gtk::RadioButton radio600(resolutionGroup, _("600 DPI (Maximum Quality)"));
    radio72.set_active(true); 

    vbox.pack_start(radio72, Gtk::PACK_SHRINK);
    vbox.pack_start(radio300, Gtk::PACK_SHRINK);
    vbox.pack_start(radio600, Gtk::PACK_SHRINK);

    exportDialog.get_content_area()->pack_start(vbox);
    exportDialog.show_all_children();

    if (exportDialog.run() != Gtk::RESPONSE_OK) {
        return; 
    }

    double selectedDpi = 72.0; 
    if (radio300.get_active()) {
        selectedDpi = 300.0;
    } else if (radio600.get_active()) {
        selectedDpi = 600.0;
    }

    handleExport("png", "mappa.png", map,
                 [](Exporter& r, std::shared_ptr<MindMap> m, const std::string& filename, double dpi){
                     r.exportToPng(m, filename, dpi);
                 }, selectedDpi);
}

void ExportManager::exportToPdf(std::shared_ptr<MindMap> map) {
    handleExport("pdf", "mappa.pdf", map,
                 [](Exporter& r, std::shared_ptr<MindMap> m, const std::string& filename, double dpi){
                     r.exportToPdf(m, filename);
                 });
}

void ExportManager::exportToFreeplane(std::shared_ptr<MindMap> map) {
    handleExport("freeplane", "mappa.mm", map,
                 [](Exporter& r, std::shared_ptr<MindMap> m, const std::string& filename, double dpi){
                     r.exportToFreeplane(m, filename);
                 });
}

void ExportManager::handleExport(const std::string& format, const std::string& default_filename,
                                std::shared_ptr<MindMap> map,
                                std::function<void(Exporter&, std::shared_ptr<MindMap>, const std::string&, double)> render_func,
                                double dpi) {
    Gtk::FileChooserAction action = Gtk::FILE_CHOOSER_ACTION_SAVE;
    std::string dialog_title;
    std::string success_message;
    std::string error_message_prefix;

    if (format == "png") {
        dialog_title = _("Export to PNG");
        success_message = _("PNG export completed successfully!");
        error_message_prefix = _("Error during PNG export: ");
    } else if (format == "pdf") {
        dialog_title = _("Export to PDF");
        success_message = _("PDF export completed successfully!");
        error_message_prefix = _("Error during PDF export: ");
    } else if (format == "freeplane") {
        dialog_title = _("Export to Freeplane");
        success_message = _("Freeplane export completed successfully!");
        error_message_prefix = _("Error during Freeplane export: ");
    }

    std::string export_filename = default_filename;
    if (map && map->root && !map->root->text.empty()) {
        std::string sanitized_map_name = map->root->text;
        for (auto& c : sanitized_map_name) {
            if (!std::isalnum(c) && c != '-' && c != '_') {
                c = '_';
            }
        }

        if (!sanitized_map_name.empty()) {
            if (format == "png") export_filename = sanitized_map_name + ".png";
            else if (format == "pdf") export_filename = sanitized_map_name + ".pdf";
            else if (format == "freeplane") export_filename = sanitized_map_name + ".mm";
        }
    }

    Gtk::FileChooserDialog fileDialog(m_parentWindow, dialog_title, action);
    fileDialog.add_button(_("Cancel"), Gtk::RESPONSE_CANCEL);
    fileDialog.add_button(_("Export"), Gtk::RESPONSE_OK);
    fileDialog.set_do_overwrite_confirmation(true);
    fileDialog.set_current_name(export_filename);

    std::string directory = getLastUsedDirectory();
    if (!directory.empty()) {
        fileDialog.set_current_folder(directory);
    }

    if (format == "freeplane") {
        auto filter = Gtk::FileFilter::create();
        filter->set_name(_("Freeplane files"));
        filter->add_pattern("*.mm");
        fileDialog.add_filter(filter);
    } else if (format == "png") {
        auto filter = Gtk::FileFilter::create();
        filter->set_name(_("PNG images"));
        filter->add_pattern("*.png");
        fileDialog.add_filter(filter);
    } else if (format == "pdf") {
        auto filter = Gtk::FileFilter::create();
        filter->set_name(_("PDF documents"));
        filter->add_pattern("*.pdf");
        fileDialog.add_filter(filter);
    }

    if (fileDialog.run() == Gtk::RESPONSE_OK) {
         Exporter r(4096, 4096);        
         try {
            std::string filename = fileDialog.get_filename();
            updateLastUsedDirectory(filename); 
            render_func(r, map, filename, dpi);
            if (m_statusCallback) m_statusCallback(success_message);
        } catch(const std::exception& e) {
            std::string error_msg = error_message_prefix + e.what();
            Gtk::MessageDialog(m_parentWindow, error_msg, false, Gtk::MESSAGE_ERROR).run();
        } catch(...) {
            Gtk::MessageDialog(m_parentWindow, error_message_prefix + _("Unknown error occurred."), false, Gtk::MESSAGE_ERROR).run();
        }
    }
}

// TODO: Refactor ConfigManager to be a Singleton or inject it properly.
// For now, we instantiate it locally or use a static map for directory (simplified).
// To keep things clean without big refactoring of ConfigManager, we'll assume
// MainWindow handles the config persistence for now, or we implement a quick fix.
// Actually, ConfigManager saves to file. We can just instantiate one.

std::string ExportManager::getLastUsedDirectory() {
    ConfigManager config; 
    return config.getLastUsedDirectory();
}

void ExportManager::updateLastUsedDirectory(const std::string& path) {
    ConfigManager config;
    std::filesystem::path p(path);
    config.saveLastUsedDirectory(p.parent_path().string());
}
