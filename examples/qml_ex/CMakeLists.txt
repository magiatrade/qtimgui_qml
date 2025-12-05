cmake_minimum_required(VERSION 3.16)

project(qml_ex VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick OpenGL)

qt_standard_project_setup(REQUIRES 6.5)

# Get the root directory of qtimgui
set(QTIMGUI_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# Add imgui library
set(IMGUI_DIR ${QTIMGUI_ROOT}/modules/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR})

# Add qt_imgui_quick library
set(QTIMGUI_SRC ${QTIMGUI_ROOT}/src)
add_library(qt_imgui_quick STATIC
    ${QTIMGUI_SRC}/ImGuiRenderer.h
    ${QTIMGUI_SRC}/ImGuiRenderer.cpp
    ${QTIMGUI_SRC}/QtImGui.h
    ${QTIMGUI_SRC}/QtImGui.cpp
    ${QTIMGUI_SRC}/ImGuiQuickItem.h
    ${QTIMGUI_SRC}/ImGuiQuickItem.cpp
)
target_include_directories(qt_imgui_quick PUBLIC ${QTIMGUI_SRC})
target_link_libraries(qt_imgui_quick PUBLIC
    imgui
    Qt6::Quick
    Qt6::OpenGL
)

qt_add_executable(appqml_ex
    main.cpp
)

qt_add_qml_module(appqml_ex
    URI qml_ex
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        ${QTIMGUI_SRC}/ImGuiQuickItem.h
        ${QTIMGUI_SRC}/ImGuiQuickItem.cpp
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appqml_ex PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appqml_ex
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appqml_ex
    PRIVATE
    Qt6::Quick
    Qt6::OpenGL
    qt_imgui_quick
)

include(GNUInstallDirs)
install(TARGETS appqml_ex
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
