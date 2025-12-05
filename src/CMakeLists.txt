cmake_minimum_required (VERSION 3.16)
project(qtimgui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Try Qt6 first, fall back to Qt5
find_package(Qt6 COMPONENTS Core Gui Quick Widgets OpenGL OpenGLWidgets QUIET)
if(Qt6_FOUND)
    set(QT_VERSION_MAJOR 6)
    message(STATUS "Using Qt6")
else()
    find_package(Qt5 COMPONENTS Core Quick Gui Widgets REQUIRED)
    set(QT_VERSION_MAJOR 5)
    message(STATUS "Using Qt5")
endif()

set(
    qt_imgui_sources
    ImGuiRenderer.h
    ImGuiRenderer.cpp
    QtImGui.h
    QtImGui.cpp
)

set(
    qt_imgui_qml_sources
    ${qt_imgui_sources}
    ImGuiQuickItem.h
    ImGuiQuickItem.cpp
)

# qt_imgui_quick: library with a qt renderer for Qml / QtQuick applications
add_library(qt_imgui_quick STATIC ${qt_imgui_qml_sources})
target_include_directories(qt_imgui_quick PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(Qt6_FOUND)
    target_link_libraries(
        qt_imgui_quick
        PUBLIC
        imgui
        Qt6::Gui
        Qt6::Quick
        Qt6::OpenGL
    )
else()
    target_link_libraries(
        qt_imgui_quick
        PUBLIC
        imgui
        Qt5::Gui
        Qt5::Quick
    )
endif()

if (ANDROID)
    target_link_libraries(qt_imgui_quick PUBLIC log dl GLESv2 z)
endif()

# qt_imgui_widget: library with a qt renderer for classic Qt Widget applications
add_library(qt_imgui_widgets STATIC ${qt_imgui_sources})
target_include_directories(qt_imgui_widgets PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(Qt6_FOUND)
    target_link_libraries(
        qt_imgui_widgets
        PUBLIC
        imgui
        Qt6::Widgets
        Qt6::OpenGLWidgets
        Qt6::OpenGL
    )
else()
    target_link_libraries(
        qt_imgui_widgets
        PUBLIC
        imgui
        Qt5::Widgets
    )
endif()

if (ANDROID)
    target_link_libraries(qt_imgui_widgets PUBLIC log dl GLESv2 z)
endif()
target_compile_definitions(qt_imgui_widgets PUBLIC QT_WIDGETS_LIB)
