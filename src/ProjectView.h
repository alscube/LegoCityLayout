#pragma once

#include <QWidget>

class QVBoxLayout;
class QStackedWidget;
class QToolBar;
class QAction;
class QLabel;

class LoadedProjects;
class CityLayoutView;
class OpenOrCreateProjectView;
class TableDefinitionEditorView;

class ProjectView final : public QWidget
{
    Q_OBJECT
public:
    explicit ProjectView(QWidget *parent = nullptr);
    ~ProjectView() override;

    void promptToCreateProject();

    void startProject(const QString &title);

    void defineTableOutline();
    void showCityLayout();
    bool saveTableDefinition();
    void promptToOpenLayout();
    void openLastLayout();
    bool openLayout(const QString &path);

    void closeProject();

    bool setCurrentProjectIndex(int index);

signals:
    void openLayoutRequested();
    void openLastLayoutRequested();

private:
    void BuildUI( );
    void AddToolBar( QVBoxLayout* layout );
    void AddViews( QVBoxLayout* layout );

    void refreshToolbar();
    void setCurrentWidget(QWidget *widget);
    QWidget *currentWidget() const;
    QByteArray tableState();
    void initializeNewProject(const QString &title);

    QWidget *m_cityLayoutHeader = nullptr;
    QLabel *m_cityLayoutTitle = nullptr;
    QStackedWidget *m_views = nullptr;
    QToolBar *m_toolbar = nullptr;
    QAction *m_tableViewAction = nullptr;
    QAction *m_cityViewAction = nullptr;
    QAction *m_unitsAction = nullptr;
    QAction *m_gridAction = nullptr;
    QAction *m_snapAction = nullptr;
    LoadedProjects &_Projects;
    CityLayoutView *m_cityLayoutView = nullptr;
    TableDefinitionEditorView *m_tableDefinitionEditor = nullptr;
    OpenOrCreateProjectView *m_openOrCreateProjectView = nullptr;
};

