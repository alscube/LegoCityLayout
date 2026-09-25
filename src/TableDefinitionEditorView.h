#pragma once

#include <QPointF>
#include <QPolygonF>
#include <QWidget>

class QKeyEvent;
class QMouseEvent;
class QPainter;
class TableDefinition;

class TableDefinitionEditorView final : public QWidget
{
    Q_OBJECT

public:
    explicit TableDefinitionEditorView(TableDefinition &definition,
                                   QWidget *parent = nullptr);

    void begin();
    void reset();
    void resetViewScale();

signals:
    void editingFinished();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void paintDefinition(QPainter &painter) const;
    QPointF snappedPoint(const QPointF &point) const;

private:
    TableDefinition &m_definition;
    QPolygonF m_draft;
    QPointF m_cursor;
    bool m_active = false;
    bool m_drawingSide = false;
    bool m_gridVisible = true;
    bool m_snapToGrid = true;
    qreal m_viewScale = 1.0;
    qsizetype m_selectedSurface = -1;
    qsizetype m_selectedSide = -1;
};
