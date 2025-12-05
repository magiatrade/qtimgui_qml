# WebAssembly Build Guide

This guide explains how to build and run the QtImGui QML project for WebAssembly, allowing it to run in web browsers.

## Prerequisites

### 1. Emscripten SDK

Qt for WebAssembly requires a specific version of Emscripten. Check which version your Qt installation needs:

```bash
# For Qt 6.9.3, the required version is 3.1.70
~/emsdk/emsdk install 3.1.70
~/emsdk/emsdk activate 3.1.70
```

To verify your Emscripten installation:
```bash
source ~/emsdk/emsdk_env.sh
emcc --version
```

### 2. Qt for WebAssembly

Install Qt with WebAssembly support using the Qt Maintenance Tool or Qt Online Installer. Select:
- **Qt 6.9.x** (or your preferred version)
- **WebAssembly (single-threaded)** - recommended for compatibility
- **WebAssembly (multi-threaded)** - optional, requires SharedArrayBuffer support

Verify installation:
```bash
ls ~/Qt/6.9.3/wasm_singlethread/bin/qt-cmake
```

## Code Adaptations

The following changes were made to support WebAssembly:

### 1. OpenGL ES / WebGL Support

WebAssembly uses WebGL (based on OpenGL ES), not desktop OpenGL. The shaders and GL calls were adapted.

**src/ImGuiRenderer.cpp** and **src/ImGuiQuickItem.cpp**:
```cpp
#if defined(ANDROID) || defined(__EMSCRIPTEN__)
#define USE_GLSL_ES
#endif

#ifdef USE_GLSL_ES
#define IMGUIRENDERER_GLSL_VERSION "#version 300 es\n"
#else
#define IMGUIRENDERER_GLSL_VERSION "#version 330\n"
#endif
```

### 2. OpenGL Context Configuration

**examples/qml_ex/main.cpp**:
```cpp
QSurfaceFormat format;
#ifdef __EMSCRIPTEN__
    // WebAssembly uses WebGL (OpenGL ES 3.0)
    format.setVersion(3, 0);
    format.setRenderableType(QSurfaceFormat::OpenGLES);
#else
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
#endif
format.setDepthBufferSize(24);
format.setStencilBufferSize(8);
QSurfaceFormat::setDefaultFormat(format);
```

### 3. CMake WebAssembly Configuration

**examples/qml_ex/CMakeLists.txt**:
```cmake
# WebAssembly specific configuration
if(EMSCRIPTEN)
    target_link_options(appqml_ex PRIVATE
        -sUSE_WEBGL2=1
        -sFULL_ES3=1
        -sALLOW_MEMORY_GROWTH=1
        -sMAX_WEBGL_VERSION=2
        -sMIN_WEBGL_VERSION=2
    )
endif()
```

### 4. QML Drag & Drop for WebAssembly

QML's `Drag.Automatic` doesn't work correctly in WebAssembly because it tries to use native browser drag & drop. Use `Drag.Internal` instead:

**IndicatorItem.qml**:
```qml
// Use Internal drag type for WebAssembly compatibility
Drag.active: dragArea.drag.active
Drag.hotSpot.x: width / 2
Drag.hotSpot.y: height / 2
Drag.dragType: Drag.Internal

MouseArea {
    id: dragArea
    anchors.fill: parent
    drag.target: root

    onReleased: function(mouse) {
        // Explicitly call drop() for Drag.Internal
        root.Drag.drop()
        // Reset position
        root.x = startPos.x
        root.y = startPos.y
    }
}
```

**Main.qml** - Use `drop.source` instead of `mimeData`:
```qml
DropArea {
    anchors.fill: parent

    onDropped: function(drop) {
        // Access properties via drop.source for Drag.Internal
        if (drop.source && drop.source.indicatorType) {
            var type = drop.source.indicatorType
            chartManager.addIndicator(type, drop.x, drop.y)
        }
    }
}
```

## Building for WebAssembly

### Step 1: Set up environment

```bash
# Activate Emscripten
source ~/emsdk/emsdk_env.sh
```

### Step 2: Create build directory

```bash
cd /path/to/qtimgui_qml
mkdir build-wasm
cd build-wasm
```

### Step 3: Configure with CMake

```bash
~/Qt/6.9.3/wasm_singlethread/bin/qt-cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -S ../examples/qml_ex \
    -B .
```

### Step 4: Build

```bash
cmake --build . --target appqml_ex
```

This generates:
- `examples/qml_ex/appqml_ex.html` - Main HTML file
- `examples/qml_ex/appqml_ex.js` - JavaScript glue code
- `examples/qml_ex/appqml_ex.wasm` - WebAssembly binary (~24MB)
- `examples/qml_ex/qtloader.js` - Qt loader script

## Running in Browser

WebAssembly files must be served via HTTP (not file://). Use any local web server:

### Using Python

```bash
cd build-wasm/examples/qml_ex
python3 -m http.server 8080
```

Then open: http://localhost:8080/appqml_ex.html

### Using Node.js

```bash
npx serve build-wasm/examples/qml_ex
```

### Using PHP

```bash
cd build-wasm/examples/qml_ex
php -S localhost:8080
```

## Troubleshooting

### "Import #0 env: module is not an object or function"

This usually means stale build files. Clean and rebuild:
```bash
rm -rf build-wasm
mkdir build-wasm
cd build-wasm
# Reconfigure and rebuild
```

### "Application exit (RuntimeError: unreachable)"

Check browser console for details. Common causes:
- OpenGL version mismatch (ensure OpenGL ES 3.0 is configured)
- Missing WebGL2 support in browser

### Emscripten version mismatch

Qt requires a specific Emscripten version. Check Qt documentation or build output for required version:
```bash
# Install specific version
~/emsdk/emsdk install 3.1.70
~/emsdk/emsdk activate 3.1.70
```

### Drag & Drop not working

- Use `Drag.Internal` instead of `Drag.Automatic`
- Call `Drag.drop()` explicitly in `onReleased`
- Access dragged item properties via `drop.source` instead of `mimeData`

### Large WASM file size

For production, consider:
- Building with `-DCMAKE_BUILD_TYPE=Release`
- Enabling WASM optimization flags
- Using gzip compression on your web server

## Browser Compatibility

Tested and working on:
- Chrome/Chromium (recommended)
- Firefox
- Safari
- Edge

Requirements:
- WebGL 2.0 support
- WebAssembly support
- ~50MB memory available

## Development Tips

1. **Hot Reload**: After rebuilding, hard refresh the browser (Ctrl+Shift+R / Cmd+Shift+R) to clear cached WASM files.

2. **Console Logging**: QML `console.log()` outputs to browser developer console.

3. **Debugging**: Use browser developer tools. Source maps are available in debug builds.

4. **Performance**: WebAssembly runs slightly slower than native. Complex ImGui/ImPlot visualizations may need optimization.

## File Structure After Build

```
build-wasm/
└── examples/
    └── qml_ex/
        ├── appqml_ex.html      # Open this in browser
        ├── appqml_ex.js        # JS glue code
        ├── appqml_ex.wasm      # WASM binary
        ├── qtloader.js         # Qt loader
        └── ...                 # Other Qt resources
```
