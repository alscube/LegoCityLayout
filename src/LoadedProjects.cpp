
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
#include "LoadedProjects.h"

#include "CityLayoutElement.h"
#include <QtMath>

LoadedProjects &LoadedProjects::instance()
{
    static LoadedProjects projects;
    return projects;
}

QString LoadedProjects::title()
{
    return currentProject() ? currentProject()->title() : QString();
}


int LoadedProjects::addProject(const QString &title)
{
    if (title.trimmed().isEmpty()) return -1;
    m_projects.push_back(std::make_unique<ProjectData>(title));
    return projectCount() - 1;
}

bool LoadedProjects::removeProject(int index)
{
    if (!project(index)) return false;
    m_projects.erase(m_projects.begin() + index);
    if (m_currentProjectIndex == index) m_currentProjectIndex = -1;
    else if (m_currentProjectIndex > index) --m_currentProjectIndex;
    return true;
}

void LoadedProjects::clear()
{
    m_currentProjectIndex = -1;
    m_projects.clear();
}

int LoadedProjects::projectCount() const { return static_cast<int>(m_projects.size()); }
int LoadedProjects::currentProjectIndex() const { return m_currentProjectIndex; }


bool LoadedProjects::setCurrentProjectIndex(int index)
{
    if (index != -1 && !project(index))
        return false;

    m_currentProjectIndex = index;

    return true;
}


ProjectData* LoadedProjects::project(int index)
{
    return index >= 0 && index < projectCount() ? m_projects[index].get() : nullptr;
}

// const ProjectData* LoadedProjects::project(int index) const
// {
//     return index >= 0 && index < projectCount() ? m_projects[index].get() : nullptr;
// }


ProjectData* LoadedProjects::currentProject()
{
    return project(m_currentProjectIndex);
}

// const ProjectData* LoadedProjects::currentProject() const
// {
//     return project(m_currentProjectIndex);
// }



TableDefinition &LoadedProjects::tableDefinition() { Q_ASSERT(currentProject()); return currentProject()->tableDefinition; }
//const TableDefinition &LoadedProjects::tableDefinition() const { Q_ASSERT(currentProject()); return currentProject()->tableDefinition; }
CityLayoutElements &LoadedProjects::cityLayouts() { Q_ASSERT(currentProject()); return currentProject()->cityLayouts; }
//const CityLayoutElements &LoadedProjects::cityLayouts() const { Q_ASSERT(currentProject()); return currentProject()->cityLayouts; }
TableEditorState& LoadedProjects::tableEditor() { Q_ASSERT(currentProject()); return currentProject()->tableEditor; }
//const TableEditorState& LoadedProjects::tableEditor() const { Q_ASSERT(currentProject()); return currentProject()->tableEditor; }

void LoadedProjects::setViewZoom(const QPointF &anchor, qreal zoomFactor)
{
    ProjectData *project = currentProject();
    if (!project) return;
    const qreal newZoom = qBound(0.25, zoomFactor, 24.0);
    const qreal relativeScale = newZoom / project->zoomFactor;
    project->tableEditor.viewScale = newZoom;
    if (qFuzzyCompare(newZoom, project->zoomFactor)) return;

    project->cityLayouts.zoomAllElements(anchor, relativeScale, newZoom);
    project->tableDefinition.scale(anchor, relativeScale);
    auto &editor = project->tableEditor;
    const auto scaledPoint = [&](const QPointF &point) {
        return anchor + (point - anchor) * relativeScale;
    };
    for (QPointF &point : editor.draft) point = scaledPoint(point);
    for (QLineF &side : editor.editSides) {
        side = QLineF(scaledPoint(side.p1()), scaledPoint(side.p2()));
    }
    editor.sideStart = scaledPoint(editor.sideStart);
    editor.cursor = scaledPoint(editor.cursor);
    project->gridOrigin = scaledPoint(project->gridOrigin);
    project->zoomFactor = newZoom;
}
