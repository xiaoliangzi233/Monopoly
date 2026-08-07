#pragma once

#include <QPointF>
#include <QPropertyAnimation>
#include <QQuickItem>

namespace neon {

class GameViewModel;

class TopDownCityView : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *viewModel READ viewModel WRITE setViewModel NOTIFY viewModelChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY cameraChanged)
    Q_PROPERTY(QPointF cameraCenter READ cameraCenter WRITE setCameraCenter NOTIFY cameraChanged)
    Q_PROPERTY(bool overviewMode READ overviewMode WRITE setOverviewMode NOTIFY overviewModeChanged)

public:
    explicit TopDownCityView(QQuickItem *parent = nullptr);
    QObject *viewModel() const;
    void setViewModel(QObject *viewModel);
    qreal zoom() const { return m_zoom; }
    void setZoom(qreal value);
    QPointF cameraCenter() const { return m_cameraCenter; }
    void setCameraCenter(const QPointF &center);
    bool overviewMode() const { return m_overviewMode; }
    void setOverviewMode(bool overview);

    Q_INVOKABLE int tileAt(qreal screenX, qreal screenY) const;
    Q_INVOKABLE void panBy(qreal screenDx, qreal screenDy);
    Q_INVOKABLE void zoomAt(qreal screenX, qreal screenY, qreal wheelDelta);
    Q_INVOKABLE void focusTile(int tileIndex);
    Q_INVOKABLE void focusCurrentPlayer();

signals:
    void viewModelChanged();
    void cameraChanged();
    void overviewModeChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    QPointF screenToWorld(const QPointF &screen) const;
    qreal effectiveZoom() const;
    QPointF effectiveCenter() const;
    void clampCamera();

    GameViewModel *m_viewModel = nullptr;
    qreal m_zoom = .86;
    QPointF m_cameraCenter{720, 610};
    bool m_overviewMode = false;
    QPropertyAnimation *m_focusAnimation = nullptr;
};

} // namespace neon
