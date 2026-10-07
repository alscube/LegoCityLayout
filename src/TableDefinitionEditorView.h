#pragma once

#include <QPointF>
#include <QPolygonF>
#include <QLineF>
#include <QList>
#include <QString>
#include <QWidget>

class LoadedProjects;
class QKeyEvent;
class QFocusEvent;
class QLabel;
class QMouseEvent;
class QPainter;
class QWheelEvent;
class QPushButton;
class TableDefinition;

class TableDefinitionEditorView final : public QWidget
{
    Q_OBJECT

public:
    explicit TableDefinitionEditorView(QWidget *parent = nullptr);

    void activateProject();
    void begin();
    void reset();
    void resetViewScale();
    void refreshMeasurementUnits();

signals:
    void editingFinished();
    void measurementUnitsChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void paintDefinition(QPainter &painter) const;
    void paintLiveMeasurement(QPainter &painter, const QLineF &side) const;
    QPointF snappedPoint(const QPointF &point) const;
    void deleteSelectedSide();
    void finishSideEditingIfClosed();
    void syncOpenSides();
    void panBy(const QPointF &offset);
    void stopPanning();

private:
    LoadedProjects &_Projects;
    QPointF m_lastPanPosition;
    Qt::MouseButton m_panButton = Qt::NoButton;
    bool m_spaceHeld = false;
};
