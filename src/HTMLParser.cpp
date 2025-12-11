#include "HTMLParser.hpp"
#include <stack>
#include <algorithm>
#include <cctype>
#include <sstream>

HTMLParser::ParsedText HTMLParser::parse(const std::string& html_content) {
    ParsedText result;
    
    std::string current_text;
    std::vector<std::pair<size_t, size_t>> bold_ranges;
    std::vector<std::pair<size_t, size_t>> code_ranges;
    std::vector<std::pair<size_t, size_t>> italic_ranges;
    std::vector<int> line_breaks;
    std::vector<std::string> tag_stack;
    
    size_t pos = 0;
    size_t len = html_content.length();
    size_t text_pos = 0; // Position in the output text
    
    // Remove the HTML head section including style
    size_t body_start = html_content.find("<body>");
    if (body_start != std::string::npos) {
        pos = body_start + 6; // Skip "<body>"
    }
    
    size_t body_end = html_content.find("</body>");
    if (body_end != std::string::npos) {
        len = body_end; // Stop at "</body>"
    }
    
    while (pos < len) {
        if (html_content[pos] == '<') {
            // Check if this is a closing tag
            bool is_closing = (pos + 1 < len && html_content[pos + 1] == '/');
            
            // Find the closing '>'
            size_t end_tag = html_content.find('>', pos);
            if (end_tag != std::string::npos) {
                std::string full_tag = html_content.substr(pos + (is_closing ? 2 : 1), end_tag - pos - (is_closing ? 2 : 1));
                
                // Extract just the tag name (without attributes)
                std::string tag_name = full_tag;
                size_t space_pos = full_tag.find(' ');
                if (space_pos != std::string::npos) {
                    tag_name = full_tag.substr(0, space_pos);
                }
                
                // If it's not a closing tag, handle opening tag
                if (!is_closing) {
                    // Handle self-closing tags
                    bool is_self_closing = (full_tag.back() == '/');
                    if (is_self_closing) {
                        tag_name = tag_name.substr(0, tag_name.length() - 1);
                    }
                    
                    if (tag_name == "br" || tag_name == "hr") {
                        current_text += "\n";
                        text_pos++;
                        line_breaks.push_back(text_pos - 1);
                    } else if (tag_name == "h1" || tag_name == "h2" || tag_name == "h3") {
                        current_text += "\n";
                        text_pos++;
                        line_breaks.push_back(text_pos - 1);
                    } else if (tag_name == "p" || tag_name == "tr") {
                        current_text += "\n";
                        text_pos++;
                        line_breaks.push_back(text_pos - 1);
                    } else if (tag_name == "pre" || tag_name == "table") {
                        current_text += "\n";
                        text_pos++;
                        line_breaks.push_back(text_pos - 1);
                    } else if (tag_name == "li") {
                        current_text += "\n";
                        // Add proper indentation based on nesting level
                        for (size_t i = 0; i < tag_stack.size(); i++) {
                            if (tag_stack[i] == "ul" || tag_stack[i] == "ol") {
                                current_text += "  "; // Two spaces per level
                            }
                        }
                        current_text += "• "; // Bullet point
                        text_pos = current_text.length();
                    }
                    
                    // Add tag to stack if it's not self-closing
                    if (!is_self_closing && 
                        tag_name != "br" && 
                        tag_name != "hr") {
                        tag_stack.push_back(tag_name);
                    }
                    
                    if (tag_name == "strong" || tag_name == "b") {
                        bold_ranges.emplace_back(text_pos, 0); // Start position, end to be set later
                    } else if (tag_name == "em" || tag_name == "i") {
                        italic_ranges.emplace_back(text_pos, 0); // Start position, end to be set later
                    } else if (tag_name == "code") {
                        code_ranges.emplace_back(text_pos, 0); // Start position, end to be set later
                    }
                } 
                // Handle closing tag
                else {
                    // Get the closing tag name
                    std::string closing_tag = tag_name;
                    
                    // Find matching opening tag in stack (from top of stack)
                    auto it = tag_stack.rbegin();
                    bool found = false;
                    while (it != tag_stack.rend()) {
                        if (*it == closing_tag) {
                            found = true;
                            break;
                        }
                        ++it;
                    }

                    if (found) {
                        if (closing_tag == "h1" || closing_tag == "h2" || closing_tag == "h3") {
                            current_text += "\n";
                            text_pos = current_text.length();
                            line_breaks.push_back(text_pos - 1);
                        } else if (closing_tag == "p" || closing_tag == "pre" || closing_tag == "table") {
                            current_text += "\n";
                            text_pos = current_text.length();
                            line_breaks.push_back(text_pos - 1);
                        } else if (closing_tag == "tr") {
                            current_text += "\n";
                            text_pos = current_text.length();
                            line_breaks.push_back(text_pos - 1);
                        } else if (closing_tag == "li") {
                            // List item ending
                        }

                        // Set end positions for formatting if applicable
                        if (closing_tag == "strong" || closing_tag == "b") {
                            if (!bold_ranges.empty() && bold_ranges.back().second == 0) {
                                bold_ranges.back().second = text_pos; // Set end position
                            }
                        } else if (closing_tag == "em" || closing_tag == "i") {
                            if (!italic_ranges.empty() && italic_ranges.back().second == 0) {
                                italic_ranges.back().second = text_pos; // Set end position
                            }
                        } else if (closing_tag == "code") {
                            if (!code_ranges.empty() && code_ranges.back().second == 0) {
                                code_ranges.back().second = text_pos; // Set end position
                            }
                        }
                    }

                    // Remove the tag from the stack if found
                    if (found) {
                        // Calculate distance from the beginning and erase the element
                        auto distance = std::distance(it, tag_stack.rend()) - 1;
                        tag_stack.erase(tag_stack.begin() + distance);
                    }
                }
                
                pos = end_tag + 1;
            } else {
                // No closing tag found, just add the character and move on
                current_text += html_content[pos];
                text_pos++;
                pos++;
            }
        } else {
            // Regular text content
            current_text += html_content[pos];
            text_pos++;
            pos++;
        }
    }
    
    result.text = current_text;
    result.bold_ranges = bold_ranges;
    result.code_ranges = code_ranges;
    result.italic_ranges = italic_ranges;
    result.line_breaks = line_breaks;
    
    return result;
}

std::string HTMLParser::strip_html(const std::string& html_content) {
    std::string result;
    size_t pos = 0;
    size_t len = html_content.length();
    
    // Remove the HTML head section including style
    size_t body_start = html_content.find("<body>");
    if (body_start != std::string::npos) {
        pos = body_start + 6; // Skip "<body>"
    }
    
    size_t body_end = html_content.find("</body>");
    if (body_end != std::string::npos) {
        len = body_end; // Stop at "</body>"
    }
    
    while (pos < len) {
        if (html_content[pos] == '<') {
            // Find the closing '>'
            size_t end_tag = html_content.find('>', pos);
            if (end_tag != std::string::npos) {
                pos = end_tag + 1;
            } else {
                // No closing tag found, just add the character and move on
                result += html_content[pos];
                pos++;
            }
        } else {
            // Regular text content
            result += html_content[pos];
            pos++;
        }
    }
    
    // Replace HTML entities
    size_t pos_entity;
    while ((pos_entity = result.find("&nbsp;")) != std::string::npos) {
        result.replace(pos_entity, 6, " ");
    }
    while ((pos_entity = result.find("&lt;")) != std::string::npos) {
        result.replace(pos_entity, 4, "<");
    }
    while ((pos_entity = result.find("&gt;")) != std::string::npos) {
        result.replace(pos_entity, 4, ">");
    }
    while ((pos_entity = result.find("&amp;")) != std::string::npos) {
        result.replace(pos_entity, 5, "&");
    }
    while ((pos_entity = result.find("&quot;")) != std::string::npos) {
        result.replace(pos_entity, 6, "\"");
    }
    while ((pos_entity = result.find("&apos;")) != std::string::npos) {
        result.replace(pos_entity, 6, "'");
    }
    
    return result;
}