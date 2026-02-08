#include "SearchManager.hpp"
#include "MapController.hpp"
#include "MapArea.hpp"
#include "Translation.hpp"
#include <algorithm>
#include <iostream>

SearchManager::SearchManager(MapController& controller, MapArea& area)
    : m_controller(controller), m_area(area) {}

void SearchManager::setStatusCallback(std::function<void(const std::string&, bool)> cb) {
    m_statusCallback = cb;
}

void SearchManager::performSearch(const std::string& query) {
    m_searchResults.clear();
    m_currentSearchIndex = 0;

    if (query.empty()) {
        if (m_statusCallback) m_statusCallback("", false);
        return;
    }

    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    auto map = m_controller.getMap();
    if (map && map->root) {
        searchRecursive(map->root, lowerQuery);
    }

    if (m_searchResults.empty()) {
        if (m_statusCallback) m_statusCallback(_("No matches found."), true);
    } else {
        if (m_statusCallback) {
            std::string msg = std::to_string(m_searchResults.size()) + _(" matches found.");
            m_statusCallback(msg, false);
        }
        highlightCurrentResult();
    }
}

void SearchManager::searchRecursive(std::shared_ptr<Node> node, const std::string& lowerQuery) {
    if (!node) return;

    std::string nodeText = node->text;
    std::transform(nodeText.begin(), nodeText.end(), nodeText.begin(), ::tolower);

    if (nodeText.find(lowerQuery) != std::string::npos) {
        m_searchResults.push_back(node);
    }

    for (auto& child : node->children) {
        searchRecursive(child, lowerQuery);
    }
}

void SearchManager::next() {
    if (m_searchResults.empty()) return;

    m_currentSearchIndex++;
    if (m_currentSearchIndex >= m_searchResults.size()) {
        m_currentSearchIndex = 0; 
    }
    highlightCurrentResult();
}

void SearchManager::prev() {
    if (m_searchResults.empty()) return;

    if (m_currentSearchIndex == 0) {
        m_currentSearchIndex = m_searchResults.size() - 1; 
    } else {
        m_currentSearchIndex--;
    }
    highlightCurrentResult();
}

void SearchManager::highlightCurrentResult() {
    if (m_searchResults.empty()) return;

    auto node = m_searchResults[m_currentSearchIndex];
    m_area.setSelectedNodes({node});
    m_area.centerViewOnNode(node);
    
    if (m_statusCallback) {
        std::string msg = _("Match ") + std::to_string(m_currentSearchIndex + 1) + 
                          "/" + std::to_string(m_searchResults.size());
        m_statusCallback(msg, false);
    }
}

void SearchManager::clear() {
    m_searchResults.clear();
    m_currentSearchIndex = 0;
    if (m_statusCallback) m_statusCallback("", false);
}
