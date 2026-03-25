#pragma once

#include <QQuickFramebufferObject>
#include <QOpenGLFramebufferObject>
#include <functional>

namespace QtImGui {

class ImGuiQuickItem : public QQuickFramebufferObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool useGlobalRender READ useGlobalRender WRITE setUseGlobalRender)

public:
    explicit ImGuiQuickItem(QQuickItem *parent = nullptr);
    ~ImGuiQuickItem() override;

    Renderer *createRenderer() const override;

    using RenderCallback = std::function<void()>;
    void setRenderCallback(RenderCallback callback);
    RenderCallback renderCallback() const { return m_renderCallback; }

    bool useGlobalRender() const { return m_useGlobalRender; }
    void setUseGlobalRender(bool v) { m_useGlobalRender = v; }

signals:
    void initialized();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void hoverMoveEvent(QHoverEvent *event) override;

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#else
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry) override;
#endif

    /// Per-instance renderer (for multi-instance input dispatch)
    void setRenderer(class ImGuiQuickItemRenderer* r) { m_renderer = r; }
    class ImGuiQuickItemRenderer* renderer() const { return m_renderer; }

private:
    RenderCallback m_renderCallback;
    bool m_useGlobalRender = true;
    class ImGuiQuickItemRenderer* m_renderer = nullptr;
    friend class ImGuiQuickItemRenderer;
};

} // namespace QtImGui
