#pragma once

#include <QQuickItem>

namespace neon {

class GameViewModel;

class IsometricBoard : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QObject *viewModel READ viewModel WRITE setViewModel NOTIFY viewModelChanged)
    Q_PROPERTY(qreal sceneScale READ sceneScale WRITE setSceneScale NOTIFY sceneScaleChanged)

public:
    explicit IsometricBoard(QQuickItem *parent = nullptr);
    QObject *viewModel() const;
    void setViewModel(QObject *viewModel);
    qreal sceneScale() const { return m_sceneScale; }
    void setSceneScale(qreal scale);
    Q_INVOKABLE int tileAt(qreal x, qreal y) const;

signals:
    void viewModelChanged();
    void sceneScaleChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;

private:
    QPointF project(const QPoint &grid) const;
    GameViewModel *m_viewModel = nullptr;
    qreal m_sceneScale = 1.0;
};

} // namespace neon
