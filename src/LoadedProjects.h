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

