# CLAUDE.md - QtImGui QML

Integracao de Dear ImGui e ImPlot com Qt Quick/QML para renderizacao de UI imediata em aplicacoes Qt.

## Visao Geral

qtimgui_qml permite usar Dear ImGui e ImPlot dentro de aplicacoes Qt Quick/QML. A renderizacao ImGui acontece em um QQuickFramebufferObject e e composited na cena QML pelo scene graph do Qt.

## Arquitetura

```
┌────────────────────────────────────────────────────────────┐
│                    QML Scene Graph                          │
│  ┌──────────────────────────────────────────────────────┐  │
│  │            ImGuiQuickItem (QML Element)               │  │
│  │                        │                              │  │
│  │                        ▼                              │  │
│  │         QQuickFramebufferObject::Renderer            │  │
│  │         (ImGuiQuickItemRenderer)                     │  │
│  │                        │                              │  │
│  │          ┌─────────────┼─────────────┐               │  │
│  │          ▼             ▼             ▼               │  │
│  │    ImGui Context  ImPlot Context  OpenGL FBO        │  │
│  │          │             │             │               │  │
│  │          └─────────────┴─────────────┘               │  │
│  │                        │                              │  │
│  │                        ▼                              │  │
│  │              FBO Texture -> QML                       │  │
│  └──────────────────────────────────────────────────────┘  │
└────────────────────────────────────────────────────────────┘
```

## Estrutura de Arquivos

```
qtimgui_qml/
├── CMakeLists.txt              # Build raiz
├── src/
│   ├── CMakeLists.txt          # Bibliotecas qt_imgui_quick/qt_imgui_widgets
│   ├── ImGuiQuickItem.h/cpp    # QQuickFramebufferObject para QML
│   ├── ImGuiRenderer.h/cpp     # Backend OpenGL para ImGui
│   └── QtImGui.h/cpp           # Helpers para Qt Widgets
├── modules/
│   ├── CMakeLists.txt          # Bibliotecas imgui e implot
│   ├── imgui/                  # Dear ImGui source
│   └── implot/                 # ImPlot source
└── examples/
    ├── qml_ex/                 # Exemplo QML
    ├── widget/                 # Exemplo Qt Widgets
    ├── window/                 # Exemplo QWindow
    └── multiple/               # Multiplas janelas
```

## Targets CMake

| Target | Descricao | Dependencias |
|--------|-----------|--------------|
| `imgui` | Biblioteca Dear ImGui | - |
| `implot` | Biblioteca ImPlot | imgui |
| `qt_imgui_quick` | Integracao QML | imgui, implot, Qt6::Quick |
| `qt_imgui_widgets` | Integracao Widgets | imgui, Qt6::Widgets |

## Uso em QML

### 1. Registrar o Tipo

No main.cpp:
```cpp
#include <ImGuiQuickItem.h>

int main(int argc, char *argv[]) {
    // ...
    qmlRegisterType<QtImGui::ImGuiQuickItem>("magiatrade_app", 1, 0, "ImGuiQuickItem");
    // ...
}
```

### 2. Usar no QML

```qml
import magiatrade_app 1.0

ApplicationWindow {
    ImGuiQuickItem {
        anchors.fill: parent
        focus: true  // Importante para input de teclado!
    }
}
```

### 3. Definir Funcao de Render

Ha duas formas de definir o que sera renderizado:

#### Opcao A: Ponteiro de funcao global (recomendado)

```cpp
// main.cpp
namespace QtImGui {
    extern void (*g_customRenderFunc)();
}

void myRenderFunc() {
    ImGui::Begin("Minha Janela");
    // ... seu codigo ImGui aqui
    ImGui::End();

    if (ImPlot::BeginPlot("Meu Grafico")) {
        // ... seu codigo ImPlot aqui
        ImPlot::EndPlot();
    }
}

int main(int argc, char *argv[]) {
    // ...
    QtImGui::g_customRenderFunc = myRenderFunc;
    // ...
}
```

#### Opcao B: Callback por instancia

```cpp
// Obter referencia ao item via findChild ou objectName
auto* imguiItem = qmlEngine->findChild<QtImGui::ImGuiQuickItem*>("myImGuiItem");
imguiItem->setRenderCallback([]() {
    ImGui::ShowDemoWindow();
});
```

### 4. Separar Logica de Render

Padrao recomendado: criar uma classe ChartRenderer

```cpp
// ChartRenderer.h
class ChartRenderer {
public:
    static ChartRenderer& instance();
    void render();
    void addData(const std::vector<double>& data);
private:
    std::vector<double> m_data;
};

// ChartRenderer.cpp
void ChartRenderer::render() {
    ImGui::Begin("Chart");
    if (ImPlot::BeginPlot("Data")) {
        ImPlot::PlotLine("Values", m_data.data(), m_data.size());
        ImPlot::EndPlot();
    }
    ImGui::End();
}

// main.cpp
QtImGui::g_customRenderFunc = []() {
    ChartRenderer::instance().render();
};
```

## OpenGL / WebGL

O renderer detecta automaticamente a plataforma e usa shaders apropriados:

```cpp
// Desktop: OpenGL 3.3 Core
#define IMGUIRENDERER_GLSL_VERSION "#version 330\n"

// WASM/Android: OpenGL ES 3.0
#define IMGUIRENDERER_GLSL_VERSION "#version 300 es\n"
```

### Configuracao de Contexto OpenGL

No main.cpp, antes de criar QGuiApplication:

```cpp
// Forcar backend OpenGL (necessario para ImGui)
QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

QSurfaceFormat format;
#ifdef __EMSCRIPTEN__
    format.setVersion(3, 0);  // OpenGL ES 3.0
    format.setRenderableType(QSurfaceFormat::OpenGLES);
#else
    format.setVersion(3, 3);  // OpenGL 3.3 Core
    format.setProfile(QSurfaceFormat::CoreProfile);
#endif
format.setDepthBufferSize(24);
format.setStencilBufferSize(8);
QSurfaceFormat::setDefaultFormat(format);
```

## Input Handling

ImGuiQuickItem captura automaticamente:

| Evento Qt | Mapeamento ImGui |
|-----------|------------------|
| mousePressEvent | io.MouseDown[button] |
| mouseReleaseEvent | io.MouseDown[button] |
| mouseMoveEvent | io.MousePos |
| wheelEvent | io.MouseWheel/H |
| keyPressEvent | io.AddKeyEvent |
| keyReleaseEvent | io.AddKeyEvent |
| hoverMoveEvent | io.MousePos |

### Teclas Especiais

Mapeamento Qt -> ImGui:
- Tab, Arrows, Page Up/Down, Home/End
- Insert, Delete, Backspace, Space
- Enter/Return, Escape
- A, C, V, X, Y, Z (para shortcuts)
- Modifiers: Ctrl, Shift, Alt, Super

### Clipboard

Integrado automaticamente com QClipboard:
```cpp
io.SetClipboardTextFn = [](void*, const char* text) {
    QGuiApplication::clipboard()->setText(text);
};
io.GetClipboardTextFn = [](void*) {
    return clipboard->text().toUtf8().data();
};
```

## Recursos ImPlot

ImPlot esta integrado e pronto para uso:

```cpp
#include <implot.h>

void render() {
    if (ImPlot::BeginPlot("Precos", ImVec2(-1, 300))) {
        // Line plot
        ImPlot::PlotLine("Close", timestamps, prices, count);

        // Candlestick (precisa implementar)
        // ImPlot::PlotCandlestick(...)

        // Barras
        ImPlot::PlotBars("Volume", timestamps, volumes, count, bar_width);

        ImPlot::EndPlot();
    }
}
```

### Tipos de Plot Disponiveis

- PlotLine, PlotScatter, PlotStairs
- PlotBars, PlotBarGroups, PlotBarH
- PlotHistogram, PlotHistogram2D
- PlotPieChart, PlotHeatmap
- PlotDigital, PlotText, PlotImage
- PlotErrorBars, PlotStems
- PlotShaded (area charts)

## Docking

Docking esta habilitado por padrao:

```cpp
// No initialize()
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
```

## Build

### Integracao em Projeto

```cmake
# Adicionar como subdirectory
add_subdirectory(qtimgui_qml)

# Linkar
target_link_libraries(meu_app PRIVATE
    qt_imgui_quick  # Para QML
    implot          # Se precisar de ImPlot diretamente
)
```

### Opcoes

```cmake
# Usar imgui/implot externo
set(QTIMGUI_BUILD_IMGUI OFF)  # Usar seu proprio imgui
set(QTIMGUI_BUILD_IMPLOT OFF) # Usar seu proprio implot
```

## WebAssembly

### Flags de Build (no CMakeLists.txt do app)

```cmake
if(EMSCRIPTEN)
    target_link_options(app PRIVATE
        -sUSE_WEBGL2=1           # WebGL 2.0
        -sFULL_ES3=1             # OpenGL ES 3.0 completo
        -sALLOW_MEMORY_GROWTH=1  # Memoria dinamica
    )
endif()
```

### Limitacoes WASM

1. **Single-threaded**: Qt WASM e single-threaded
2. **SharedArrayBuffer**: Requer headers COOP/COEP se precisar de threading
3. **File access**: Limitado, use fetch ou upload de arquivos

## Troubleshooting

### Tela preta / nada renderiza
1. Verifique se `QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL)` foi chamado
2. Verifique se o formato OpenGL esta correto para a plataforma
3. Verifique se `g_customRenderFunc` ou renderCallback foi definido

### Input nao funciona
1. Certifique-se de que `focus: true` esta no QML
2. O item precisa ter `forceActiveFocus()` em eventos de mouse

### Crash no WASM
1. Verifique se usando GLSL ES (`#version 300 es`)
2. Verifique se `precision mediump float` nos fragment shaders

### FPS baixo
1. O timer interno roda a 60 FPS (~16ms)
2. Reduza complexidade do render se necessario
3. Use `ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove` para janelas estaticas

## API Reference

### ImGuiQuickItem

```cpp
class ImGuiQuickItem : public QQuickFramebufferObject {
    Q_OBJECT
    QML_ELEMENT

public:
    // Definir callback de render por instancia
    void setRenderCallback(std::function<void()> callback);

signals:
    void initialized();  // Emitido apos contexto OpenGL criado
};
```

### QtImGui Namespace

```cpp
namespace QtImGui {
    // Ponteiro global para funcao de render customizada
    extern void (*g_customRenderFunc)();

    // Para Qt Widgets (nao QML)
    RenderRef initialize(QWidget* window, bool defaultRender = true);
    RenderRef initialize(QWindow* window, bool defaultRender = true);
    void newFrame(RenderRef ref = nullptr);
    void render(RenderRef ref = nullptr);
}
```

## Exemplo Completo

```cpp
// main.cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <ImGuiQuickItem.h>
#include <imgui.h>
#include <implot.h>

namespace QtImGui {
    extern void (*g_customRenderFunc)();
}

std::vector<float> g_data = {1, 2, 4, 3, 5, 4, 6, 5, 7};

void renderChart() {
    ImGui::Begin("Chart Window");

    if (ImPlot::BeginPlot("My Plot", ImVec2(-1, -1))) {
        ImPlot::PlotLine("Data", g_data.data(), g_data.size());
        ImPlot::EndPlot();
    }

    ImGui::End();
}

int main(int argc, char *argv[]) {
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    QSurfaceFormat::setDefaultFormat(format);

    QGuiApplication app(argc, argv);

    qmlRegisterType<QtImGui::ImGuiQuickItem>("MyApp", 1, 0, "ImGuiQuickItem");

    QtImGui::g_customRenderFunc = renderChart;

    QQmlApplicationEngine engine;
    engine.load(QUrl("qrc:/Main.qml"));

    return app.exec();
}
```

```qml
// Main.qml
import QtQuick
import MyApp 1.0

Window {
    visible: true
    width: 800
    height: 600

    ImGuiQuickItem {
        anchors.fill: parent
        focus: true
    }
}
```
