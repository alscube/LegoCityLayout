#pragma once

#include <QPointF>
#include <QPolygonF>

class QKeyEvent;
class QMouseEvent;
class QPainter;
class QWidget;
class TableDefinition;

class TableDefinitionEditor final
{
public:
    explicit TableDefinitionEditor(TableDefinition &definition);

    void begin(QWidget *view);
    void reset(QWidget *view);
    void resetViewScale();
    void translate(const QPointF &offset);
    void scale(const QPointF &anchor, qreal factor);

    bool mousePressEvent(QWidget *view, QMouseEvent *event);
    bool mouseMoveEvent(QWidget *view, QMouseEvent *event);
    bool mouseReleaseEvent(QWidget *view, QMouseEvent *event);
    bool keyPressEvent(QWidget *view, QKeyEvent *event);
    void paint(QPainter &painter) const;

private:
    TableDefinition &m_definition;
    QPolygonF m_draft;
    QPointF m_cursor;
    bool m_active = false;
    bool m_drawingSide = false;
    qreal m_viewScale = 1.0;
    qsizetype m_selectedSurface = -1;
    qsizetype m_selectedSide = -1;
};
