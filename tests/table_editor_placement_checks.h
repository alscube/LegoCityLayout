#pragma once
#include "TableDefinitionEditorView.h"
#include <QMouseEvent>

static int checkTableEditorPlacements(LoadedProjects &projects)
{
    const int original = projects.currentProjectIndex();
    const int index = projects.addProject("Table editor placement checks");
    projects.setCurrentProjectIndex(index);
    CityLayoutView city;
    TableDefinitionEditorView editor;
    struct Cleanup {
        LoadedProjects &projects;
        int index, original;
        ~Cleanup() { projects.removeProject(index); projects.setCurrentProjectIndex(original); }
    } cleanup{projects, index, original};
    projects.tableDefinition().addSurface(TableSurface("Table", QPolygonF(QRectF(0,0,800,400))));
    QPixmap artwork(32,32);
    artwork.fill(Qt::green);
    for (const QPointF &position : {QPointF(100.125,100.375), QPointF(200.125,100.375)}) {
        auto *element = new CityLayoutElement("Building", artwork, QSize(32,32),
            LegoGrid::pixelsPerStud, projects.currentProject()->zoomFactor, &city);
        element->setLayoutPosition(position);
        projects.cityLayouts().append(element);
    }
    const QPointF initial = projects.cityLayouts().first()->layoutPosition();
    const QPointF separation = projects.cityLayouts().last()->layoutPosition() - initial;
    const QPointF initialOrigin = projects.currentProject()->gridOrigin;
    editor.begin();
    // Extending the right-hand table edge must leave existing placements anchored.
    projects.tableDefinition().setSurface(0, TableSurface("Table", QPolygonF(QRectF(0,0,1200,400))));
    const auto mouse = [&](QEvent::Type type, QPointF position, Qt::MouseButton button,
                           Qt::MouseButtons buttons) {
        QMouseEvent event(type, position, position, button, buttons, Qt::NoModifier);
        QApplication::sendEvent(&editor, &event);
    };
    const auto pan = [&](QPointF step) {
        QPointF position(60,60);
        mouse(QEvent::MouseButtonPress, position, Qt::MiddleButton, Qt::MiddleButton);
        for (int i=0; i<40; ++i) {
            position += step;
            mouse(QEvent::MouseMove, position, Qt::NoButton, Qt::MiddleButton);
        }
        mouse(QEvent::MouseButtonRelease, position, Qt::MiddleButton, Qt::NoButton);
    };
    pan(QPointF(0.25,-0.25));
    const QPointF offset(10,-10);
    const auto close = [](QPointF a, QPointF b) { return QLineF(a,b).length()<1e-5; };
    if (!close(projects.cityLayouts().first()->layoutPosition(), initial+offset)
        || !close(projects.currentProject()->gridOrigin, initialOrigin+offset)
        || !close(projects.cityLayouts().last()->layoutPosition()
                    - projects.cityLayouts().first()->layoutPosition(), separation)) {
        qWarning()<<"Table editor panning shifted placements relative to the table";
        return 120;
    }
    projects.setViewZoom(QPointF(33.25,47.75), 3.0);
    projects.setViewZoom(QPointF(33.25,47.75), 1.875);
    pan(QPointF(-0.25,0.25));
    city.activateProject();
    if (!close(projects.cityLayouts().first()->layoutPosition(),initial)
        || !close(projects.currentProject()->gridOrigin,initialOrigin)
        || projects.tableDefinition().usableArea().boundingRect()!=QRectF(0,0,1200,400)) return 121;
    qInfo()<<"PASS: table extension, fractional editor panning, zoom and return preserve placements";
    return 0;
}
