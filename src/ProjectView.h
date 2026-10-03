#pragma once

#include "TableDefinition.h"

#include <QStackedWidget>

class CityLayoutView;
class OpenOrCreateProjectView;
class TableDefinitionEditorView;

// this class manages the main project view,
// which includes the open/create project view,
// the table definition editor, and the city layout view


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
    bool saveTableDefinition();
    void closeProject();

signals:
//    void projectStarted(const QString &title);
    void openLayoutRequested();
    void openLastLayoutRequested();

private:
    QByteArray tableState() const;
    QByteArray m_savedTableState;
    quint64 m_savedLayoutRevision = 0;

    void initializeNewProject(const QString &title);

    TableDefinition m_tableDefinition;
    OpenOrCreateProjectView *m_openOrCreateProjectView = nullptr;
    CityLayoutView *m_cityLayoutView = nullptr;
    TableDefinitionEditorView *m_tableDefinitionEditor = nullptr;
};
