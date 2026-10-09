
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

#include "ProjectData.h"
#include <memory>
#include <vector>

// Owns all loaded projects and resolves data access through the current index.
class LoadedProjects final
{
public:
    static LoadedProjects &instance();

    LoadedProjects(const LoadedProjects &) = delete;
    LoadedProjects &operator=(const LoadedProjects &) = delete;
    LoadedProjects(LoadedProjects &&) = delete;
    LoadedProjects &operator=(LoadedProjects &&) = delete;

    // Adds a new project with the given title.
    // Returns the index of the new project, or
    // -1 if the title is empty.
    int addProject(const QString &title);

    QString title();

    bool removeProject(int index);
    void clear();
    int projectCount() const;
    int currentProjectIndex() const;
    bool setCurrentProjectIndex(int index);
    void setViewZoom(const QPointF &anchor, qreal zoomFactor);

    ProjectData* project(int index);
//    const ProjectData* project(int index) const;

    ProjectData* currentProject();
//    const ProjectData* currentProject() const;

    TableDefinition &tableDefinition();
//    const TableDefinition &tableDefinition() const;

    CityLayoutElements &cityLayouts();
//    const CityLayoutElements &cityLayouts() const;

    TableEditorState &tableEditor();
//    const TableEditorState &tableEditor() const;

private:
    LoadedProjects() = default;
    ~LoadedProjects() = default;

    std::vector<std::unique_ptr<ProjectData>> m_projects;
    int m_currentProjectIndex = -1;
};

