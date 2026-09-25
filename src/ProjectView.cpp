#include "ProjectView.h"

#include "CityLayoutView.h"
#include "OpenOrCreateProjectView.h"
#include "TableDefinitionEditorView.h"

#include <QMainWindow>


ProjectView::ProjectView(QWidget *parent)
    : QStackedWidget(parent)
    , m_openOrCreateProjectView(new OpenOrCreateProjectView(this))
    , m_cityLayoutView(new CityLayoutView(m_tableDefinition, this))
    , m_tableDefinitionEditor(
          new TableDefinitionEditorView(m_tableDefinition, this))
{
    addWidget(m_openOrCreateProjectView);
    addWidget(m_cityLayoutView);
    addWidget(m_tableDefinitionEditor);
    setCurrentWidget(m_openOrCreateProjectView);

    connect(m_tableDefinitionEditor, &TableDefinitionEditorView::editingFinished,
            this, &ProjectView::showCityLayout);

    connect(m_openOrCreateProjectView,
            &OpenOrCreateProjectView::projectTitleAccepted,
            this, &ProjectView::initializeNewProject);

    connect(m_openOrCreateProjectView,
            &OpenOrCreateProjectView::openLayoutRequested,
            this, &ProjectView::openLayoutRequested);

    connect(m_openOrCreateProjectView,
            &OpenOrCreateProjectView::openLastLayoutRequested,
            this, &ProjectView::openLastLayoutRequested);
}

void ProjectView::promptToCreateProject()
{
    m_openOrCreateProjectView->createNewLayout();
}


void ProjectView::initializeNewProject(const QString &title)
{
    startProject(title);
    defineTableOutline();

    auto *mainWindow = qobject_cast<QMainWindow *>(window());
    if (mainWindow)
        mainWindow->setWindowTitle( tr("Lego City Layout - %1").arg(title) );

    // emit projectStarted(title);
}


void ProjectView::startProject(const QString &title)
{
    m_tableDefinition.clear();
    m_tableDefinitionEditor->reset();
    m_tableDefinitionEditor->resetViewScale();

    m_cityLayoutView->startProject(title);
}


QString ProjectView::projectTitle() const
{
    return m_cityLayoutView->projectTitle();
}


void ProjectView::defineTableOutline()
{
    setCurrentWidget(m_tableDefinitionEditor);
    m_tableDefinitionEditor->begin();
}


void ProjectView::showCityLayout()
{
    setCurrentWidget(m_cityLayoutView);
    m_cityLayoutView->setFocus(Qt::OtherFocusReason);
    m_cityLayoutView->update();
}
