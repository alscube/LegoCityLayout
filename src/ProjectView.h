#pragma once

#include "TableDefinition.h"

#include <QStackedWidget>

class CityLayoutView;
class OpenOrCreateProjectView;
class TableDefinitionEditorView;

class ProjectView final : public QStackedWidget
{
    Q_OBJECT

public:
    explicit ProjectView(QWidget *parent = nullptr);

    void promptToCreateProject();
    void startProject(const QString &title);
    QString projectTitle() const;
    void defineTableOutline();
    void showCityLayout();

signals:
//b    void projectStarted(const QString &title);
    void openLayoutRequested();
    void openLastLayoutRequested();

private:
    void initializeNewProject(const QString &title);

    TableDefinition m_tableDefinition;
    OpenOrCreateProjectView *m_openOrCreateProjectView = nullptr;
    CityLayoutView *m_cityLayoutView = nullptr;
    TableDefinitionEditorView *m_tableDefinitionEditor = nullptr;
};
