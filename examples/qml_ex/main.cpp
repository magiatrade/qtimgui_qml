#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSurfaceFormat>

#include "ChartDataManager.h"
#include "ChartRenderer.h"

// Global pointer for ImGuiQuickItem to access
ChartDataManager* g_chartManager = nullptr;

// Global render function for ImGuiQuickItem
void globalImGuiRender()
{
    ChartRenderer::render(g_chartManager);
}

// Declare the external function pointer from ImGuiQuickItem
namespace QtImGui {
    extern void (*g_customRenderFunc)();
}

int main(int argc, char *argv[])
{
    // Force OpenGL rendering backend for Qt Quick (required for ImGui)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    // Set up OpenGL format
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

    QGuiApplication app(argc, argv);

    // Create the chart data manager
    ChartDataManager chartManager;
    g_chartManager = &chartManager;

    // Set the global render function
    QtImGui::g_customRenderFunc = globalImGuiRender;

    QQmlApplicationEngine engine;

    // Expose chart manager to QML
    engine.rootContext()->setContextProperty("chartManager", &chartManager);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("qml_ex", "Main");

    return app.exec();
}
