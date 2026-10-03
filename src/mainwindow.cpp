#include "mainwindow.h"

#include "LayoutElementsView.h"
#include "ProjectView.h"
#include "SettingsDialog.h"
#include "TableDefinitionEditorView.h"
#include "UserSettings.h"

#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , _splitter(new QSplitter(Qt::Horizontal, this))
    , _projectView(new ProjectView(_splitter))
{
    SetupMainMenu( );

    CreateSplitterView( );

    setWindowTitle(tr("Lego City Layout"));

    // restore application size and position
    const QByteArray savedGeometry = UserSettings::instance().mainWindowGeometry();
    if (!savedGeometry.isEmpty()) {
        restoreGeometry(savedGeometry);
    }
    else
        resize(1000, 700);
}


MainWindow::~MainWindow()
{
    UserSettings::instance().setMainWindowGeometry(saveGeometry());
    UserSettings::instance().setSplitterState(_splitter->saveState());
}


void MainWindow::SetupMainMenu( )
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *newProjectAction = fileMenu->addAction(tr("&New..."));
    newProjectAction->setShortcut(QKeySequence::New);
    connect(newProjectAction, &QAction::triggered,
            _projectView, &ProjectView::promptToCreateProject);

    fileMenu->addSeparator();
    QAction *closeProjectAction = fileMenu->addAction(tr("&Close"));
    closeProjectAction->setShortcut(QKeySequence::Close);
    connect(closeProjectAction, &QAction::triggered,
            _projectView, &ProjectView::closeProject);

    QAction *saveTableAction = fileMenu->addAction(tr("&Save Lego Layout..."));
    saveTableAction->setShortcut(QKeySequence::Save);
    connect(saveTableAction, &QAction::triggered,
            _projectView, &ProjectView::saveTableDefinition);

    QMenu *tableMenu = menuBar()->addMenu(tr("&View"));

    QAction *defineTableAction = tableMenu->addAction(tr("Edit Table Outline..."));
    connect(defineTableAction, &QAction::triggered,
            this, &MainWindow::defineTableOutline);

    QAction *showLayoutAction = tableMenu->addAction(tr("Show City Layout"));
    connect(showLayoutAction, &QAction::triggered, this, &MainWindow::showCityLayout);

    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    QAction *settingsAction = settingsMenu->addAction(tr("Settings..."));
    settingsAction->setMenuRole(QAction::PreferencesRole);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);

    connect(_projectView, &ProjectView::openLayoutRequested,
            this, [this] {
                statusBar()->showMessage(tr("Opening layouts is not available yet"), 3000);
            });
    connect(_projectView, &ProjectView::openLastLayoutRequested,
            this, [this] {
                statusBar()->showMessage(tr("There is no saved layout to open yet"), 3000);
            });
}


void MainWindow::CreateSplitterView( )
{
    // left side: layout elements view
    _layoutElementsView = new LayoutElementsView();
    _layoutElementsView->addLayoutElements( );
    _splitter->addWidget( _layoutElementsView );

    // right side: project view
    _splitter->addWidget(_projectView);
    _splitter->setChildrenCollapsible(false);
    _splitter->setHandleWidth(8);
    _splitter->setStyleSheet(
        "QSplitter::handle { background-color: #707070; }"
        "QSplitter::handle:hover { background-color: #3d8ec9; }"
        );

    _splitter->setSizes({500, 500});

    // restore splitter state from preferences if available
    const QByteArray savedState = UserSettings::instance().splitterState();
    if (!savedState.isEmpty()) {
        _splitter->restoreState(savedState);
    }

    setCentralWidget(_splitter);
}


void MainWindow::defineTableOutline()
{
    if (_projectView->projectTitle().isEmpty()) {
        statusBar()->showMessage(tr("Please load or create a new layout before defining its table"));
        return;
    }

    _projectView->defineTableOutline();
}


void MainWindow::showCityLayout()
{
    _projectView->showCityLayout();
}


void MainWindow::showSettings()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        for (QWidget *widget : QApplication::allWidgets()) {
            if (auto *editor = qobject_cast<TableDefinitionEditorView *>(widget)) {
                editor->refreshMeasurementUnits();
            }
            widget->update();
        }
    }
}
