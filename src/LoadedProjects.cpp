#include "LoadedProjects.h"

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
