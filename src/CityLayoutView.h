#pragma once

#include <QList>
#include <QPoint>
#include <QWidget>

class QDragEnterEvent;
class QDropEvent;
class QContextMenuEvent;
class QEvent;
class QKeyEvent;
class QLabel;
class QMouseEvent;
class QWheelEvent;

class CityLayoutView final : public QWidget
{
    Q_OBJECT

public:
    explicit CityLayoutView(QWidget *parent = nullptr);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    QLabel *imageAt(const QPoint &position) const;
    void setHoveredImage(QLabel *image);
    void setSelectedImage(QLabel *image);
    void updateImageHighlight(QLabel *image);
    void deleteImage(QLabel *image);

    QList<QLabel *> m_images;
    QLabel *m_draggedImage = nullptr;
    QLabel *m_hoveredImage = nullptr;
    QLabel *m_selectedImage = nullptr;
    QPoint m_dragOffset;
    QPoint m_lastPanPosition;
    bool m_isPanning = false;
    qreal m_zoomFactor = 1.0;
};
