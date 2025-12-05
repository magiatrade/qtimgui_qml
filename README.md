# QtImGui - Qt Backend for Dear ImGui

A Qt backend integration for [Dear ImGui](https://github.com/ocornut/imgui) that enables ImGui to run within Qt applications, including **full QML support**.

[![QtImGui Demo](https://i.gyazo.com/eb68699c96b9147cca3d5ea9fadfc263.gif)](https://gyazo.com/eb68699c96b9147cca3d5ea9fadfc263)

## Features

- **Qt 6 Support** - Full compatibility with Qt 6.x (tested with 6.9.2)
- **QML Integration** - Native QML item (`ImGuiQuickItem`) for seamless integration with Qt Quick applications
- **QOpenGLWidget/Window Support** - Traditional Qt OpenGL widget and window backends
- **ImPlot Integration** - Built-in support for [ImPlot](https://github.com/epezent/implot) charting library
- **Docking Support** - ImGui docking branch enabled for flexible window layouts
- **Retina/HiDPI Support** - Proper device pixel ratio handling for high-resolution displays
- **Cross-platform** - Works on macOS, Windows, and Linux

## Table of Contents

- [Quick Start](#quick-start)
- [Architecture Overview](#architecture-overview)
- [QML Integration](#qml-integration)
- [Project Structure](#project-structure)
- [Building](#building)
- [Examples](#examples)
- [Technical Details](#technical-details)
- [Troubleshooting](#troubleshooting)
- [Roadmap](#roadmap)
- [Contributing](#contributing)

## Quick Start

### Prerequisites

- Qt 6.5+ (recommended: Qt 6.9.2)
- CMake 3.16+
- C++17 compiler
- OpenGL 3.3+ support

### Clone with Submodules

```bash
git clone --recursive https://github.com/seanchas116/qtimgui.git
cd qtimgui
```

If you already cloned without `--recursive`:

```bash
git submodule update --init --recursive
cd modules/imgui
git checkout docking  # Enable docking support
cd ../..
```

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Architecture Overview

### Why This Project Exists

Dear ImGui is an immediate-mode GUI library designed for game development tools and debug interfaces. However, integrating ImGui with Qt applications—especially Qt Quick/QML—requires careful handling of:

1. **OpenGL Context Sharing** - Qt manages its own OpenGL contexts
2. **Event Translation** - Qt events must be translated to ImGui events
3. **Render Loop Integration** - ImGui rendering must fit into Qt's render pipeline
4. **HiDPI Handling** - Device pixel ratios must be properly managed

This project solves these challenges by providing backend implementations that bridge Qt and ImGui.

### Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        Qt Application                            │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────────┐  │
│  │   QML UI     │    │ QOpenGLWidget│    │  QOpenGLWindow   │  │
│  │  (Main.qml)  │    │   Backend    │    │     Backend      │  │
│  └──────┬───────┘    └──────┬───────┘    └────────┬─────────┘  │
│         │                   │                      │            │
│         ▼                   ▼                      ▼            │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    ImGuiQuickItem                         │  │
│  │         (QQuickFramebufferObject-based wrapper)           │  │
│  └──────────────────────────┬───────────────────────────────┘  │
│                             │                                   │
│                             ▼                                   │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    ImGuiRenderer                          │  │
│  │              (OpenGL rendering backend)                   │  │
│  └──────────────────────────┬───────────────────────────────┘  │
│                             │                                   │
│                             ▼                                   │
│  ┌────────────────┐    ┌────────────────┐                      │
│  │    Dear ImGui  │    │     ImPlot     │                      │
│  │   (docking)    │◄───┤   (charting)   │                      │
│  └────────────────┘    └────────────────┘                      │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

## QML Integration

### The Challenge

Qt Quick uses a scene graph renderer that operates differently from traditional OpenGL widgets. QML items don't have direct access to an OpenGL context, making ImGui integration non-trivial.

### The Solution: QQuickFramebufferObject

We implemented `ImGuiQuickItem` based on `QQuickFramebufferObject`, which:

1. **Creates an offscreen FBO** - ImGui renders to a framebuffer object
2. **Integrates with Qt's scene graph** - The FBO texture is composited into the QML scene
3. **Handles coordinate transformation** - Properly maps between QML and ImGui coordinate spaces

### Usage in QML

```qml
import QtQuick
import qml_ex  // Your module name

ApplicationWindow {
    width: 1200
    height: 800
    visible: true

    ImGuiQuickItem {
        anchors.fill: parent
        focus: true  // Important for keyboard input
    }
}
```

### Custom Rendering

To render your own ImGui content, set a global render function:

```cpp
// In your main.cpp
namespace QtImGui {
    extern void (*g_customRenderFunc)();
}

void myImGuiRender() {
    ImGui::Begin("My Window");
    ImGui::Text("Hello from QML!");
    ImGui::End();
}

int main(int argc, char *argv[]) {
    // ... Qt setup ...

    QtImGui::g_customRenderFunc = myImGuiRender;

    // ... load QML ...
}
```

## Project Structure

```
qtimgui/
├── CMakeLists.txt              # Root CMake configuration
├── README.md                   # This file
├── src/
│   ├── ImGuiRenderer.h/cpp     # Core OpenGL rendering backend
│   ├── ImGuiQuickItem.h/cpp    # QML integration (QQuickFramebufferObject)
│   ├── QtImGui.h/cpp           # Widget/Window backend helpers
│   └── CMakeLists.txt
├── modules/
│   ├── imgui/                  # Dear ImGui (docking branch)
│   └── implot/                 # ImPlot charting library
└── examples/
    ├── widget/                 # QOpenGLWidget example
    ├── window/                 # QOpenGLWindow example
    └── qml_ex/                 # QML + Drag & Drop example
        ├── main.cpp
        ├── Main.qml
        ├── IndicatorItem.qml
        ├── ChartDataManager.h/cpp
        ├── ChartRenderer.h/cpp
        └── CMakeLists.txt
```

## Building

### CMake Options

```cmake
# In your CMakeLists.txt
add_subdirectory(path/to/qtimgui)

# Link against the appropriate target:
target_link_libraries(your_app PRIVATE
    qt_imgui_quick  # For QML/Qt Quick apps
    # or
    qt_imgui_widget # For QOpenGLWidget apps
    implot          # If using ImPlot
)
```

### Standalone Build

```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/6.9.2/macos ..
cmake --build .

# Run the QML example
./examples/qml_ex/appqml_ex
```

## Examples

### QML Example (qml_ex)

A complete demonstration of QML + ImGui integration featuring:

- **Drag & Drop** - Drag indicators from QML toolbar into ImGui chart
- **ImPlot Charts** - Multiple chart types (Line, Bar, Scatter, Candlestick)
- **Docking** - Rearrangeable window layout
- **QML ↔ C++ Communication** - Data flows between QML and ImGui

#### Key Components

| File | Purpose |
|------|---------|
| `main.cpp` | Application setup, global render function |
| `Main.qml` | QML UI layout with toolbar and ImGui item |
| `IndicatorItem.qml` | Draggable indicator component |
| `ChartDataManager` | Manages chart data series (Q_OBJECT) |
| `ChartRenderer` | ImGui/ImPlot rendering logic |

## Technical Details

### Input Handling

Input events are translated from Qt to ImGui:

```cpp
// Key mapping (ImGuiQuickItem.cpp)
ImGuiKey qtKeyToImGuiKey(int qtKey) {
    switch (qtKey) {
        case Qt::Key_Tab: return ImGuiKey_Tab;
        case Qt::Key_Left: return ImGuiKey_LeftArrow;
        // ... more mappings
    }
}

// Using the new ImGui 1.87+ API
io.AddKeyEvent(ImGuiMod_Ctrl, event->modifiers() & Qt::MetaModifier);
io.AddKeyEvent(imgui_key, pressed);
```

### HiDPI / Retina Display Support

Device pixel ratio is handled in the projection matrix and framebuffer scaling:

```cpp
qreal dpr = window()->devicePixelRatio();
io.DisplayFramebufferScale = ImVec2(dpr, dpr);
```

**Important**: We removed the deprecated `ScaleClipRects()` call which caused double-scaling on Retina displays.

### FBO Y-Axis Correction

Qt Quick renders FBOs upside-down relative to OpenGL conventions. We correct this in the projection matrix:

```cpp
// Flipped projection for QQuickFramebufferObject
const float ortho_projection[4][4] = {
    { 2.0f/io.DisplaySize.x, 0.0f,                  0.0f, 0.0f },
    { 0.0f,                  2.0f/io.DisplaySize.y, 0.0f, 0.0f },  // Note: positive Y
    { 0.0f,                  0.0f,                 -1.0f, 0.0f },
    {-1.0f,                 -1.0f,                  0.0f, 1.0f },
};
```

### Docking Setup

Docking is enabled in the initialization:

```cpp
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
```

And a DockSpace is created in the render function:

```cpp
ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
```

## Troubleshooting

### Common Issues

#### "Black screen / Nothing renders"

1. Ensure OpenGL backend is set before creating QGuiApplication:
   ```cpp
   QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
   ```

2. Set proper OpenGL format:
   ```cpp
   QSurfaceFormat format;
   format.setVersion(3, 3);
   format.setProfile(QSurfaceFormat::CoreProfile);
   QSurfaceFormat::setDefaultFormat(format);
   ```

#### "Rendering is upside down"

This was fixed by adjusting the projection matrix. Ensure you're using the latest `ImGuiQuickItem.cpp`.

#### "Double-sized / Blurry on Retina"

The deprecated `ScaleClipRects()` was removed. If you see this issue, ensure you're not calling it.

#### "Keyboard input not working"

Make sure the ImGuiQuickItem has focus:
```qml
ImGuiQuickItem {
    focus: true
}
```

#### "Mouse coordinates are wrong"

Check that device pixel ratio is properly applied and that mouse events use `event->position()` (Qt 6) instead of `event->pos()`.

## Roadmap

### Short-term Improvements

- [ ] **Multiple ImGui Contexts** - Support multiple ImGuiQuickItem instances with separate contexts
- [ ] **Viewport Support** - Enable ImGui viewports for windows outside the main QML window
- [ ] **Custom Fonts** - Easy API for loading custom fonts with proper DPI scaling
- [ ] **Touch Support** - Better touch input handling for mobile/tablet

### Medium-term Features

- [ ] **Theme System** - QML-exposed properties for ImGui theming
- [ ] **Property Bindings** - Direct QML property bindings to ImGui widgets
- [ ] **ImGui Widget Library** - QML components that wrap common ImGui patterns
- [ ] **Performance Profiling** - Built-in performance overlay

### Long-term Vision

- [ ] **Vulkan Backend** - Support Qt's Vulkan renderer (Qt 6.6+)
- [ ] **Metal Backend** - Native Metal rendering on macOS/iOS
- [ ] **ImGui Node Editor** - Integration with [imgui-node-editor](https://github.com/thedmd/imgui-node-editor)
- [ ] **Scripting Support** - Lua/Python bindings for runtime ImGui scripting

### Potential Applications

| Application | Description |
|-------------|-------------|
| **Trading Platforms** | Real-time charts with ImPlot, dockable panels |
| **Game Dev Tools** | Scene editors, asset browsers, debug overlays |
| **Data Visualization** | Interactive dashboards with QML + ImPlot |
| **Audio/Video Editors** | Timeline editors, waveform displays |
| **Scientific Apps** | Data analysis tools with custom plots |

## Contributing

Contributions are welcome! Here's how to get started:

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Make your changes
4. Test with the examples
5. Submit a pull request

### Development Guidelines

- Follow the existing code style
- Add comments for non-obvious code
- Update documentation for new features
- Test on multiple platforms if possible

## License

This project follows the same license as Dear ImGui (MIT).

## Acknowledgments

- [Omar Cornut](https://github.com/ocornut) - Dear ImGui creator
- [Evan Pezent](https://github.com/epezent) - ImPlot creator
- [seanchas116](https://github.com/seanchas116) - Original QtImGui implementation
- All contributors who helped improve this project

---

**Questions?** Open an issue on GitHub or check the [ImGui FAQ](https://github.com/ocornut/imgui/blob/master/docs/FAQ.md).
