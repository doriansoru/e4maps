# e4maps User Guide

Welcome to **e4maps**, a simple and efficient mind mapping application designed to help you organize your thoughts and ideas visually.

## Table of Contents

1. [Getting Started](#getting-started)

2. [Basic Operations](#basic-operations)

3. [Working with Nodes](#working-with-nodes)

4. [Customizing Appearance](#customizing-appearance)

5. [Exporting](#exporting)

6. [Keyboard Shortcuts](#keyboard-shortcuts)

## Getting Started

### Installation

Please refer to the `README.md` file included with the source code for detailed instructions on how to build and install e4maps on Linux and Windows.

### Launching the Application

Once installed or built, you can launch the application from your terminal or desktop environment.

```bash
./e4maps
```

## Basic Operations

The main window provides a clean interface with a header bar containing frequently used tools.

- **New Map**: Click the "New Document" icon (page with a plus) to start a fresh mind map.

- **Open Map**: Click the "Open" icon (folder) to load an existing `.e4m` file.

- **Save Map**: Click the "Save" icon (floppy disk) to save your current work. Use the menu for "Save As...".

- **Navigation**:

    - **Zoom In/Out**: Use `Ctrl + +` / `Ctrl + -` or the View menu options.

    - **Reset View**: Use `Ctrl + 0` to center the map and reset zoom.

    - **Pan**: Click and drag on the empty background to move the map around.

## Working with Nodes

Nodes are the building blocks of your mind map.

### Adding and Removing

- **Add Branch**: Select a node and press `Tab` or click the "+" button in the header bar to add a child node.

- **Remove Branch**: Select a node (or multiple) and press `Delete` or click the "-" button in the header bar to remove them. *Note: The root node cannot be removed.*

### Editing Nodes

Double-click on any node (or select it and press Enter) to open the **Node Editor**. Here you can customize:

- **Text**: The label of the node.

- **Font**: Custom font family, style, and size.

- **Text Color**: Color of the text.

- **Connection Color**: Color of the line connecting to this node.

- **Images**:

    - **Node Image**: Add an image to be displayed inside the node. You can adjust its width and height.

    - **Connection Image**: Add an icon to the connection line.

- **Connection Label**: Add text to the connecting line.

### Organizing

- **Cut/Copy/Paste**: Use standard shortcuts (`Ctrl+X`, `Ctrl+C`, `Ctrl+V`) or the menu to move or duplicate subtrees. You can select multiple nodes to copy/paste them together.

## Customizing Appearance

You can change the look and feel of your entire map using the **Theme Editor**.

Access it via the Menu button (hamburger icon) -> **Edit Theme...**.

Themes work by **Levels**:

- **Level 0**: The Root node.

- **Level 1**: Direct children of the root.

- **Level 2+**: Subsequent descendants.

For each level, you can customize:

- **Shape**: Corner radius, padding.

- **Colors**: Background (normal and hover), border, shadow.

- **Typography**: Font and text color.

- **Connections**: Line color, width, and style (dashed or solid).

- **Effects**: Shadow offset and blur.

## Exporting

Share your mind maps by exporting them to different formats. Go to Menu -> **Export**:

- **To PNG**: Save as an image. You can choose the quality:

    - 72 DPI (Screen)

    - 300 DPI (High Quality)

    - 600 DPI (Print Quality)

- **To PDF**: Save as a document.

- **To Freeplane**: Export as a `.mm` file for compatibility with Freeplane software.

## Keyboard Shortcuts

| Action | Shortcut |
|--------|----------|
| **Add Node** | `Tab` |
| | |
| **Remove Node** | `Delete` |
| | |
| **Undo** | `Ctrl + Z` |
| | |
| **Redo** | `Ctrl + Shift + Z` |
| | |
| **Cut** | `Ctrl + X` |
| | |
| **Copy** | `Ctrl + C` |
| | |
| **Paste** | `Ctrl + V` |
| | |
| **Zoom In** | `Ctrl + +` |
| | |
| **Zoom Out** | `Ctrl + -` |
| | |
| **Reset View** | `Ctrl + 0` |
| | |
| **Save As** | `Ctrl + Shift + S` |
| | |
| **Quit** | `Ctrl + Q` |

---

*Happy Mapping!*