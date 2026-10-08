#pragma once

#include "CityLayoutElements.h"
#include "TableDefinition.h"
#include <QByteArray>

class TableEditorState
{
public:
    QPolygonF draft;
    QList<QLineF> editSides;
    QString draftSurfaceName;
    QPointF sideStart;
    QPointF cursor;
    bool active = false;
    bool drawingSide = false;
    bool editingIndividualSides = false;
    bool gridVisible = true;
    bool snapToGrid = true;
    qreal viewScale = 1.875;
    qsizetype selectedSurface = -1;
    qsizetype selectedSide = -1;
};


class ProjectData
{
public:
    explicit ProjectData(const QString &title);
    ~ProjectData();

    ProjectData(const ProjectData &) = delete;
    ProjectData &operator=(const ProjectData &) = delete;

    QString title();
    void setTitle(const QString &title);

    TableDefinition tableDefinition;
    CityLayoutElements cityLayouts;
    TableEditorState tableEditor;
    bool editingTable = false;
    QPointF gridOrigin;
    qreal zoomFactor = 1.875;
    quint64 changeRevision = 0;
    QByteArray savedTableState;
    quint64 savedLayoutRevision = 0;

private:
    QString _ProjectTitle;
};
