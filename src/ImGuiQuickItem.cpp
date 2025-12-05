#include "ImGuiQuickItem.h"
#include "ImGuiRenderer.h"
#include <implot.h>

#include <QQuickWindow>
#include <QOpenGLFramebufferObject>
#include <QOpenGLContext>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QCursor>
#include <QGuiApplication>
#include <QClipboard>
#include <QDateTime>
#include <QTimer>

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QOpenGLExtraFunctions>
#else
#include <QOpenGLExtraFunctions>
#endif

#ifdef ANDROID
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#define USE_GLSL_ES
#endif

#ifdef USE_GLSL_ES
#define IMGUIRENDERER_GLSL_VERSION "#version 300 es\n"
#else
#define IMGUIRENDERER_GLSL_VERSION "#version 330\n"
#endif

namespace QtImGui {

// Global custom render function pointer - can be set by application
void (*g_customRenderFunc)() = nullptr;

namespace {

ImGuiKey qtKeyToImGuiKey(int qtKey)
{
    switch (qtKey) {
        case Qt::Key_Tab: return ImGuiKey_Tab;
        case Qt::Key_Left: return ImGuiKey_LeftArrow;
        case Qt::Key_Right: return ImGuiKey_RightArrow;
        case Qt::Key_Up: return ImGuiKey_UpArrow;
        case Qt::Key_Down: return ImGuiKey_DownArrow;
        case Qt::Key_PageUp: return ImGuiKey_PageUp;
        case Qt::Key_PageDown: return ImGuiKey_PageDown;
        case Qt::Key_Home: return ImGuiKey_Home;
        case Qt::Key_End: return ImGuiKey_End;
        case Qt::Key_Insert: return ImGuiKey_Insert;
        case Qt::Key_Delete: return ImGuiKey_Delete;
        case Qt::Key_Backspace: return ImGuiKey_Backspace;
        case Qt::Key_Space: return ImGuiKey_Space;
        case Qt::Key_Enter: return ImGuiKey_Enter;
        case Qt::Key_Return: return ImGuiKey_Enter;
        case Qt::Key_Escape: return ImGuiKey_Escape;
        case Qt::Key_A: return ImGuiKey_A;
        case Qt::Key_C: return ImGuiKey_C;
        case Qt::Key_V: return ImGuiKey_V;
        case Qt::Key_X: return ImGuiKey_X;
        case Qt::Key_Y: return ImGuiKey_Y;
        case Qt::Key_Z: return ImGuiKey_Z;
        default: return ImGuiKey_None;
    }
}

QByteArray g_currentClipboardText;

} // namespace

class ImGuiQuickItemRenderer : public QQuickFramebufferObject::Renderer, protected QOpenGLExtraFunctions
{
public:
    ImGuiQuickItemRenderer()
        : m_item(nullptr)
        , g_Time(0.0)
        , g_MouseWheel(0.0f)
        , g_MouseWheelH(0.0f)
        , g_FontTexture(0)
        , g_ShaderHandle(0)
        , g_VertHandle(0)
        , g_FragHandle(0)
        , g_AttribLocationTex(0)
        , g_AttribLocationProjMtx(0)
        , g_AttribLocationPosition(0)
        , g_AttribLocationUV(0)
        , g_AttribLocationColor(0)
        , g_VboHandle(0)
        , g_VaoHandle(0)
        , g_ElementsHandle(0)
        , g_ctx(nullptr)
        , m_initialized(false)
    {
        for (int i = 0; i < 3; i++) {
            g_MousePressed[i] = false;
        }
    }

    ~ImGuiQuickItemRenderer() override
    {
        if (g_ctx) {
            ImPlot::DestroyContext();
            ImGui::DestroyContext(g_ctx);
        }
    }

    void initialize()
    {
        if (m_initialized) return;

        initializeOpenGLFunctions();

        g_ctx = ImGui::CreateContext();
        ImGui::SetCurrentContext(g_ctx);
        ImPlot::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
        io.BackendPlatformName = "qtimgui_qml";

        // Enable docking
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        io.SetClipboardTextFn = [](void *, const char *text) {
            QGuiApplication::clipboard()->setText(text);
        };
        io.GetClipboardTextFn = [](void *) {
            g_currentClipboardText = QGuiApplication::clipboard()->text().toUtf8();
            return (const char *)g_currentClipboardText.data();
        };

        createDeviceObjects();
        m_initialized = true;
    }

    bool createFontsTexture()
    {
        ImGui::SetCurrentContext(g_ctx);

        ImGuiIO& io = ImGui::GetIO();
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        GLint last_texture;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGenTextures(1, &g_FontTexture);
        glBindTexture(GL_TEXTURE_2D, g_FontTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        io.Fonts->TexID = (void *)(size_t)g_FontTexture;
        glBindTexture(GL_TEXTURE_2D, last_texture);

        return true;
    }

    bool createDeviceObjects()
    {
        ImGui::SetCurrentContext(g_ctx);

        GLint last_texture, last_array_buffer, last_vertex_array;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);

        const GLchar *vertex_shader =
            IMGUIRENDERER_GLSL_VERSION
            "uniform mat4 ProjMtx;\n"
            "in vec2 Position;\n"
            "in vec2 UV;\n"
            "in vec4 Color;\n"
            "out vec2 Frag_UV;\n"
            "out vec4 Frag_Color;\n"
            "void main()\n"
            "{\n"
            "   Frag_UV = UV;\n"
            "   Frag_Color = Color;\n"
            "   gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
            "}\n";

        const GLchar* fragment_shader =
            IMGUIRENDERER_GLSL_VERSION
            "precision mediump float;"
            "uniform sampler2D Texture;\n"
            "in vec2 Frag_UV;\n"
            "in vec4 Frag_Color;\n"
            "out vec4 Out_Color;\n"
            "void main()\n"
            "{\n"
            "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
            "}\n";

        g_ShaderHandle = glCreateProgram();
        g_VertHandle = glCreateShader(GL_VERTEX_SHADER);
        g_FragHandle = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(g_VertHandle, 1, &vertex_shader, 0);
        glShaderSource(g_FragHandle, 1, &fragment_shader, 0);
        glCompileShader(g_VertHandle);
        glCompileShader(g_FragHandle);
        glAttachShader(g_ShaderHandle, g_VertHandle);
        glAttachShader(g_ShaderHandle, g_FragHandle);
        glLinkProgram(g_ShaderHandle);

        g_AttribLocationTex = glGetUniformLocation(g_ShaderHandle, "Texture");
        g_AttribLocationProjMtx = glGetUniformLocation(g_ShaderHandle, "ProjMtx");
        g_AttribLocationPosition = glGetAttribLocation(g_ShaderHandle, "Position");
        g_AttribLocationUV = glGetAttribLocation(g_ShaderHandle, "UV");
        g_AttribLocationColor = glGetAttribLocation(g_ShaderHandle, "Color");

        glGenBuffers(1, &g_VboHandle);
        glGenBuffers(1, &g_ElementsHandle);

        glGenVertexArrays(1, &g_VaoHandle);
        glBindVertexArray(g_VaoHandle);
        glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
        glEnableVertexAttribArray(g_AttribLocationPosition);
        glEnableVertexAttribArray(g_AttribLocationUV);
        glEnableVertexAttribArray(g_AttribLocationColor);

#define OFFSETOF(TYPE, ELEMENT) ((size_t)&(((TYPE *)0)->ELEMENT))
        glVertexAttribPointer(g_AttribLocationPosition, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, pos));
        glVertexAttribPointer(g_AttribLocationUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, uv));
        glVertexAttribPointer(g_AttribLocationColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)OFFSETOF(ImDrawVert, col));
#undef OFFSETOF

        createFontsTexture();

        glBindTexture(GL_TEXTURE_2D, last_texture);
        glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
        glBindVertexArray(last_vertex_array);

        return true;
    }

    void renderDrawList(ImDrawData *draw_data)
    {
        ImGui::SetCurrentContext(g_ctx);

        const ImGuiIO& io = ImGui::GetIO();
        int fb_width = (int)(io.DisplaySize.x * io.DisplayFramebufferScale.x);
        int fb_height = (int)(io.DisplaySize.y * io.DisplayFramebufferScale.y);
        if (fb_width == 0 || fb_height == 0)
            return;
        // Note: ScaleClipRects() is deprecated since ImGui 1.80+
        // Clip rects are scaled in the rendering loop using draw_data->FramebufferScale

        GLint last_active_texture; glGetIntegerv(GL_ACTIVE_TEXTURE, &last_active_texture);
        glActiveTexture(GL_TEXTURE0);
        GLint last_program; glGetIntegerv(GL_CURRENT_PROGRAM, &last_program);
        GLint last_texture; glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_texture);
        GLint last_array_buffer; glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_array_buffer);
        GLint last_element_array_buffer; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_element_array_buffer);
        GLint last_vertex_array; glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vertex_array);
        GLint last_blend_src_rgb; glGetIntegerv(GL_BLEND_SRC_RGB, &last_blend_src_rgb);
        GLint last_blend_dst_rgb; glGetIntegerv(GL_BLEND_DST_RGB, &last_blend_dst_rgb);
        GLint last_blend_src_alpha; glGetIntegerv(GL_BLEND_SRC_ALPHA, &last_blend_src_alpha);
        GLint last_blend_dst_alpha; glGetIntegerv(GL_BLEND_DST_ALPHA, &last_blend_dst_alpha);
        GLint last_blend_equation_rgb; glGetIntegerv(GL_BLEND_EQUATION_RGB, &last_blend_equation_rgb);
        GLint last_blend_equation_alpha; glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &last_blend_equation_alpha);
        GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
        GLint last_scissor_box[4]; glGetIntegerv(GL_SCISSOR_BOX, last_scissor_box);
        GLboolean last_enable_blend = glIsEnabled(GL_BLEND);
        GLboolean last_enable_cull_face = glIsEnabled(GL_CULL_FACE);
        GLboolean last_enable_depth_test = glIsEnabled(GL_DEPTH_TEST);
        GLboolean last_enable_scissor_test = glIsEnabled(GL_SCISSOR_TEST);

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);

        glViewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
        // Flip Y axis for QQuickFramebufferObject (Qt flips the FBO texture)
        const float ortho_projection[4][4] =
        {
            { 2.0f/io.DisplaySize.x, 0.0f,                  0.0f, 0.0f },
            { 0.0f,                  2.0f/io.DisplaySize.y, 0.0f, 0.0f },
            { 0.0f,                  0.0f,                 -1.0f, 0.0f },
            {-1.0f,                 -1.0f,                  0.0f, 1.0f },
        };
        glUseProgram(g_ShaderHandle);
        glUniform1i(g_AttribLocationTex, 0);
        glUniformMatrix4fv(g_AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);
        glBindVertexArray(g_VaoHandle);

        ImVec2 clip_off = draw_data->DisplayPos;
        ImVec2 clip_scale = draw_data->FramebufferScale;

        for (int n = 0; n < draw_data->CmdListsCount; n++)
        {
            const ImDrawList* cmd_list = draw_data->CmdLists[n];
            const ImDrawIdx* idx_buffer_offset = 0;

            glBindBuffer(GL_ARRAY_BUFFER, g_VboHandle);
            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ElementsHandle);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

            for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
            {
                const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
                if (pcmd->UserCallback)
                {
                    pcmd->UserCallback(cmd_list, pcmd);
                }
                else
                {
                    ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                    ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                    if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                        continue;

                    // Scissor Y is flipped because we flipped the projection matrix
                    glScissor((int)clip_min.x, (int)clip_min.y, (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y));
                    glBindTexture(GL_TEXTURE_2D, (GLuint)(size_t)pcmd->GetTexID());
                    glDrawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, idx_buffer_offset + pcmd->IdxOffset);
                }
            }
        }

        glUseProgram(last_program);
        glBindTexture(GL_TEXTURE_2D, last_texture);
        glActiveTexture(last_active_texture);
        glBindVertexArray(last_vertex_array);
        glBindBuffer(GL_ARRAY_BUFFER, last_array_buffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, last_element_array_buffer);
        glBlendEquationSeparate(last_blend_equation_rgb, last_blend_equation_alpha);
        glBlendFuncSeparate(last_blend_src_rgb, last_blend_dst_rgb, last_blend_src_alpha, last_blend_dst_alpha);
        if (last_enable_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (last_enable_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (last_enable_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (last_enable_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
        glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);
        glScissor(last_scissor_box[0], last_scissor_box[1], (GLsizei)last_scissor_box[2], (GLsizei)last_scissor_box[3]);
    }

    void newFrame()
    {
        if (!m_item) return;

        ImGui::SetCurrentContext(g_ctx);

        ImGuiIO& io = ImGui::GetIO();

        io.DisplaySize = ImVec2(m_item->width(), m_item->height());
        qreal dpr = m_item->window() ? m_item->window()->devicePixelRatio() : 1.0;
        io.DisplayFramebufferScale = ImVec2(dpr, dpr);

        double current_time = QDateTime::currentMSecsSinceEpoch() / double(1000);
        io.DeltaTime = g_Time > 0.0 ? (float)(current_time - g_Time) : (float)(1.0f/60.0f);
        if (io.DeltaTime <= 0.0f) io.DeltaTime = 0.00001f;
        g_Time = current_time;

        for (int i = 0; i < 3; i++) {
            io.MouseDown[i] = g_MousePressed[i];
        }

        io.MouseWheelH = g_MouseWheelH;
        io.MouseWheel = g_MouseWheel;
        g_MouseWheelH = 0;
        g_MouseWheel = 0;

        ImGui::NewFrame();
    }

    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override
    {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setSamples(4);
        return new QOpenGLFramebufferObject(size, format);
    }

    void synchronize(QQuickFramebufferObject *item) override
    {
        m_item = static_cast<ImGuiQuickItem*>(item);

        if (!m_initialized) {
            initialize();
        }

        // Copy mouse position from item
        ImGui::SetCurrentContext(g_ctx);
        ImGuiIO& io = ImGui::GetIO();
        io.MousePos = ImVec2(m_mousePos.x(), m_mousePos.y());
    }

    void render() override
    {
        if (!m_item || !m_initialized) return;

        // Clear the framebuffer before rendering
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        newFrame();

        // Call the render callback if set
        if (m_item->renderCallback()) {
            m_item->renderCallback();
        } else if (g_customRenderFunc) {
            // Use global custom render function if set
            g_customRenderFunc();
        } else {
            // Default demo UI
            ImGui::ShowDemoWindow();
            ImPlot::ShowDemoWindow();
        }

        ImGui::Render();
        renderDrawList(ImGui::GetDrawData());

        // Request update for continuous rendering
        update();
    }

    void setMousePressed(int button, bool pressed)
    {
        if (button >= 0 && button < 3) {
            g_MousePressed[button] = pressed;
        }
    }

    void setMousePos(const QPointF &pos)
    {
        m_mousePos = pos;
    }

    void addMouseWheel(float x, float y)
    {
        g_MouseWheelH += x;
        g_MouseWheel += y;
    }

    void handleKeyEvent(QKeyEvent *event, bool pressed)
    {
        if (!g_ctx) return;

        ImGui::SetCurrentContext(g_ctx);
        ImGuiIO& io = ImGui::GetIO();

        // Handle modifier keys using the new API
#ifdef Q_OS_MAC
        io.AddKeyEvent(ImGuiMod_Ctrl, event->modifiers() & Qt::MetaModifier);
        io.AddKeyEvent(ImGuiMod_Shift, event->modifiers() & Qt::ShiftModifier);
        io.AddKeyEvent(ImGuiMod_Alt, event->modifiers() & Qt::AltModifier);
        io.AddKeyEvent(ImGuiMod_Super, event->modifiers() & Qt::ControlModifier);
#else
        io.AddKeyEvent(ImGuiMod_Ctrl, event->modifiers() & Qt::ControlModifier);
        io.AddKeyEvent(ImGuiMod_Shift, event->modifiers() & Qt::ShiftModifier);
        io.AddKeyEvent(ImGuiMod_Alt, event->modifiers() & Qt::AltModifier);
        io.AddKeyEvent(ImGuiMod_Super, event->modifiers() & Qt::MetaModifier);
#endif

        // Handle regular keys
        ImGuiKey imgui_key = qtKeyToImGuiKey(event->key());
        if (imgui_key != ImGuiKey_None) {
            io.AddKeyEvent(imgui_key, pressed);
        }

        if (pressed) {
            const QString text = event->text();
            if (text.size() == 1) {
                io.AddInputCharacter(text.at(0).unicode());
            }
        }
    }

private:
    ImGuiQuickItem *m_item;
    QPointF m_mousePos;

    double g_Time;
    bool g_MousePressed[3];
    float g_MouseWheel;
    float g_MouseWheelH;
    GLuint g_FontTexture;
    int g_ShaderHandle, g_VertHandle, g_FragHandle;
    int g_AttribLocationTex, g_AttribLocationProjMtx;
    int g_AttribLocationPosition, g_AttribLocationUV, g_AttribLocationColor;
    unsigned int g_VboHandle, g_VaoHandle, g_ElementsHandle;
    ImGuiContext* g_ctx;
    bool m_initialized;
};

// Static renderer pointer for input forwarding
static ImGuiQuickItemRenderer* s_currentRenderer = nullptr;

ImGuiQuickItem::ImGuiQuickItem(QQuickItem *parent)
    : QQuickFramebufferObject(parent)
    , m_renderCallback(nullptr)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setFlag(ItemAcceptsInputMethod);
    setFlag(ItemIsFocusScope);
    setFocus(true);

    // Enable continuous updates
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &QQuickFramebufferObject::update);
    timer->start(16); // ~60 FPS
}

ImGuiQuickItem::~ImGuiQuickItem()
{
    if (s_currentRenderer) {
        s_currentRenderer = nullptr;
    }
}

QQuickFramebufferObject::Renderer *ImGuiQuickItem::createRenderer() const
{
    auto renderer = new ImGuiQuickItemRenderer();
    s_currentRenderer = renderer;
    return renderer;
}

void ImGuiQuickItem::setRenderCallback(RenderCallback callback)
{
    m_renderCallback = callback;
}

void ImGuiQuickItem::mousePressEvent(QMouseEvent *event)
{
    forceActiveFocus();
    if (s_currentRenderer) {
        int button = 0;
        if (event->button() == Qt::RightButton) button = 1;
        else if (event->button() == Qt::MiddleButton) button = 2;
        s_currentRenderer->setMousePressed(button, true);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        s_currentRenderer->setMousePos(event->position());
#else
        s_currentRenderer->setMousePos(event->localPos());
#endif
    }
    update();
}

void ImGuiQuickItem::mouseReleaseEvent(QMouseEvent *event)
{
    if (s_currentRenderer) {
        int button = 0;
        if (event->button() == Qt::RightButton) button = 1;
        else if (event->button() == Qt::MiddleButton) button = 2;
        s_currentRenderer->setMousePressed(button, false);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        s_currentRenderer->setMousePos(event->position());
#else
        s_currentRenderer->setMousePos(event->localPos());
#endif
    }
    update();
}

void ImGuiQuickItem::mouseMoveEvent(QMouseEvent *event)
{
    if (s_currentRenderer) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        s_currentRenderer->setMousePos(event->position());
#else
        s_currentRenderer->setMousePos(event->localPos());
#endif
    }
    update();
}

void ImGuiQuickItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    mousePressEvent(event);
}

void ImGuiQuickItem::wheelEvent(QWheelEvent *event)
{
    if (s_currentRenderer) {
        float x = 0, y = 0;
        if (event->pixelDelta().x() != 0) {
            x = event->pixelDelta().x() / 120.0f;
        } else {
            x = event->angleDelta().x() / 120.0f;
        }
        if (event->pixelDelta().y() != 0) {
            y = event->pixelDelta().y() / 120.0f;
        } else {
            y = event->angleDelta().y() / 120.0f;
        }
        s_currentRenderer->addMouseWheel(x, y);
    }
    update();
}

void ImGuiQuickItem::keyPressEvent(QKeyEvent *event)
{
    if (s_currentRenderer) {
        s_currentRenderer->handleKeyEvent(event, true);
    }
    update();
}

void ImGuiQuickItem::keyReleaseEvent(QKeyEvent *event)
{
    if (s_currentRenderer) {
        s_currentRenderer->handleKeyEvent(event, false);
    }
    update();
}

void ImGuiQuickItem::hoverMoveEvent(QHoverEvent *event)
{
    if (s_currentRenderer) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        s_currentRenderer->setMousePos(event->position());
#else
        s_currentRenderer->setMousePos(event->pos());
#endif
    }
    update();
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void ImGuiQuickItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickFramebufferObject::geometryChange(newGeometry, oldGeometry);
    update();
}
#else
void ImGuiQuickItem::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickFramebufferObject::geometryChanged(newGeometry, oldGeometry);
    update();
}
#endif

} // namespace QtImGui
