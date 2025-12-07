#ifndef CONFIG_MANAGER_HPP
#define CONFIG_MANAGER_HPP

#include <string>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>

class ConfigManager {
private:
    std::string m_configDir;
    std::string m_recentFile;
    std::deque<std::string> m_recentFiles;

public:
    ConfigManager() {
        initializeConfigDir();
        m_recentFile = (std::filesystem::path(m_configDir) / "recent.txt").string();
        loadRecentFiles();
    }

    void initializeConfigDir() {
        const char* home = getenv("HOME");
        std::filesystem::path configDir = std::filesystem::path(home ? home : ".") / ".config" / "e4maps";
        if (!std::filesystem::exists(configDir)) {
            std::filesystem::create_directories(configDir);
        }
        m_configDir = configDir.string();
    }

    std::string getConfigDir() const { return m_configDir; }

    // Recent files functionality
    void loadRecentFiles() {
        m_recentFiles.clear();
        std::ifstream in(m_recentFile);
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty()) m_recentFiles.push_back(line);
        }
    }

    void saveRecentFiles() {
        std::ofstream out(m_recentFile);
        for (const auto& f : m_recentFiles) out << f << std::endl;
    }

    void addToRecent(const std::string& path) {
        // Remove if already exists
        auto it = std::remove(m_recentFiles.begin(), m_recentFiles.end(), path);
        m_recentFiles.erase(it, m_recentFiles.end());
        // Add to front
        m_recentFiles.push_front(path);
        // Limit to 5
        if (m_recentFiles.size() > 5) m_recentFiles.resize(5);
        saveRecentFiles();
    }

    const std::deque<std::string>& getRecentFiles() const { return m_recentFiles; }
};

#endif // CONFIG_MANAGER_HPP