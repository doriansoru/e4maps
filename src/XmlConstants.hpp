#ifndef XML_CONSTANTS_HPP
#define XML_CONSTANTS_HPP

namespace E4Maps {
namespace Xml {

// Tags
constexpr const char* TAG_MAP = "mindmap";
constexpr const char* TAG_NODE = "node";

// Attributes - Common
constexpr const char* ATTR_TEXT = "text";
constexpr const char* ATTR_FONT = "font";
constexpr const char* ATTR_VERSION = "version";

// Attributes - Image
constexpr const char* ATTR_IMG = "img";
constexpr const char* ATTR_IMG_WIDTH = "iw";
constexpr const char* ATTR_IMG_HEIGHT = "ih";

// Attributes - Connection to parent
constexpr const char* ATTR_CONN_TEXT = "ctext";
constexpr const char* ATTR_CONN_FONT = "conn_font";
constexpr const char* ATTR_CONN_IMG = "cimg";

// Attributes - Position & Style
constexpr const char* ATTR_R = "r";
constexpr const char* ATTR_G = "g";
constexpr const char* ATTR_B = "b";
constexpr const char* ATTR_TEXT_R = "tr";
constexpr const char* ATTR_TEXT_G = "tg";
constexpr const char* ATTR_TEXT_B = "tb";
constexpr const char* ATTR_X = "x";
constexpr const char* ATTR_Y = "y";
constexpr const char* ATTR_MANUAL = "manual";

// Attributes - Overrides
constexpr const char* ATTR_OVR_COLOR = "ovr_c";
constexpr const char* ATTR_OVR_TEXT_COLOR = "ovr_t";
constexpr const char* ATTR_OVR_FONT = "ovr_f";
constexpr const char* ATTR_OVR_CONN_FONT = "ovr_cf";

// Arbitrary Connections (if implemented in XML yet)
constexpr const char* TAG_CONNECTIONS = "connections";
constexpr const char* TAG_CONNECTION = "connection";
constexpr const char* ATTR_FROM = "from";
constexpr const char* ATTR_TO = "to";

} // namespace Xml
} // namespace E4Maps

#endif // XML_CONSTANTS_HPP