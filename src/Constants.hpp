#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

namespace E4Maps {

// Node and UI constants
constexpr double NODE_PADDING = 8.0;
constexpr double NODE_MARGIN = 20.0;
constexpr double MIN_ZOOM = 0.1;
constexpr double MAX_ZOOM = 5.0;
constexpr double ZOOM_FACTOR_IN = 1.1;
constexpr double ZOOM_FACTOR_OUT = 1.0/1.1;
constexpr double DEFAULT_NODE_RADIUS = 160.0;
constexpr double BRANCH_ANNOTATION_PADDING = 3.0;

// Image constants
constexpr int DEFAULT_MAX_IMAGE_DIM = 150;
constexpr int BRANCH_ICON_SIZE = 32;

// Arrow constants
constexpr double DEFAULT_ARROW_SIZE = 20.0;
constexpr double MIN_ARROW_SIZE = 12.0;

// Export constants
constexpr double EXPORT_MARGIN = 50.0;
constexpr double DEFAULT_DPI = 72.0;
constexpr double HIGH_DPI = 300.0;
constexpr double MAX_DPI = 600.0;

// Layout constants
constexpr double INITIAL_RADIUS = 160.0;
constexpr double ANGLE_OFFSET = 0.3;

// Command history
constexpr size_t MAX_COMMAND_HISTORY = 50;

} // namespace E4Maps

#endif // CONSTANTS_HPP