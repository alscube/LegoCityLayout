#pragma once

#include "CityLayoutElements.h"
#include "TableDefinition.h"
#include "TableDefinitionEditor.h"

#include <QPalette>
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
class CityLayoutElement;


class CityLayoutView final : public QWidget
{
    Q_OBJECT

public:
    explicit CityLayoutView(QWidget *parent = nullptr);

    void startProject(const QString &title);
    QString projectTitle() const;

    // TableDefinition &tableDefinition();
    // const TableDefinition &tableDefinition() const;
    void beginTableDefinition();

    bool mouseDragged() const { return m_mouseDragged; }
    void setMouseDragged(bool dragged) { m_mouseDragged = dragged; }

    QPoint mousePressPosition() const { return m_mousePressPosition; }
    // void mousePressPosition(const QPoint &position) { m_mousePressPosition = position; }

    QPoint dragOffset() const { return m_dragOffset; }
    void setDragOffset(const QPoint &offset) { m_dragOffset = offset; }

signals:
    void createNewLayoutRequested();
    void openLayoutRequested();
    void openLastLayoutRequested();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
//    void leaveEvent(QEvent *event) override;

private:
    void panBy(const QPoint &offset);

    CityLayoutElements _LayoutElements;
    TableDefinition m_tableDefinition;
    TableDefinitionEditor m_tableDefinitionEditor;
    QPalette m_layoutPalette;
    QLabel *m_titleLabel = nullptr;
    QWidget *m_startupPanel = nullptr;
    QPoint m_dragOffset;
    QPoint m_mousePressPosition;
    QPoint m_lastPanPosition;
    QPointF m_gridOrigin;
    bool m_mouseDragged = false;
    bool m_isPanning = false;
    qreal m_zoomFactor = 1.0;
};
