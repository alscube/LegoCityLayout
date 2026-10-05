#pragma once

#include <QPointF>
#include <QPolygonF>
#include <QLineF>
#include <QList>
#include <QString>
#include <QWidget>

class LoadedProjects;
class QKeyEvent;
class QLabel;
class QMouseEvent;
class QPainter;
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
    void paintEvent(QPaintEvent *event) override;

private:
    void paintDefinition(QPainter &painter) const;
    QPointF snappedPoint(const QPointF &point) const;
    void deleteSelectedSide();
    void finishSideEditingIfClosed();
    void syncOpenSides();

private:
    LoadedProjects &_Projects;
};
