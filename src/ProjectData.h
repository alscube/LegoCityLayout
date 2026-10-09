
// Copyright 2026. Alan Krzywicki

// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
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
