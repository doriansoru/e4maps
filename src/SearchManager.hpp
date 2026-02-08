#ifndef SEARCHMANAGER_HPP
#define SEARCHMANAGER_HPP

#include <vector>
#include <memory>
#include <string>
#include <functional>

// Forward declarations
class MapController;
class MapArea;
class Node;

class SearchManager {
public:
    SearchManager(MapController& controller, MapArea& area);

    // Perform a new search
    void performSearch(const std::string& query);

    // Navigate results
    void next();
    void prev();
    
    // Clear search results
    void clear();

    // Set callback for status updates (message, isError)
    void setStatusCallback(std::function<void(const std::string&, bool)> cb);

private:
    MapController& m_controller;
    MapArea& m_area;
    
    std::vector<std::shared_ptr<Node>> m_searchResults;
    size_t m_currentSearchIndex = 0;
    
    std::function<void(const std::string&, bool)> m_statusCallback;

    void highlightCurrentResult();
    void searchRecursive(std::shared_ptr<Node> node, const std::string& lowerQuery);
};

#endif // SEARCHMANAGER_HPP
