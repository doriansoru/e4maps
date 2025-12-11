#ifndef HTMLPARSER_HPP
#define HTMLPARSER_HPP

#include <string>
#include <vector>
#include <utility>

class HTMLParser {
public:
    struct ParsedText {
        std::string text;
        std::vector<std::pair<size_t, size_t>> bold_ranges;      // pairs of start/end positions for bold text
        std::vector<std::pair<size_t, size_t>> code_ranges;      // pairs of start/end positions for code text
        std::vector<std::pair<size_t, size_t>> italic_ranges;    // pairs of start/end positions for italic text
        std::vector<int> line_breaks;                            // positions where line breaks should occur
    };

    static ParsedText parse(const std::string& html_content);
    static std::string strip_html(const std::string& html_content);

private:
    static void process_element(const std::string& tag, bool opening, size_t& text_pos, 
                                std::string& current_text, 
                                std::vector<std::pair<size_t, size_t>>& bold_ranges,
                                std::vector<std::pair<size_t, size_t>>& code_ranges,
                                std::vector<std::pair<size_t, size_t>>& italic_ranges,
                                std::vector<int>& line_breaks,
                                std::vector<std::string>& tag_stack);
};

#endif // HTMLPARSER_HPP