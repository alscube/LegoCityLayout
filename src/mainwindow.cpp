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
    , m_splitter(new QSplitter(Qt::Horizontal, this))
    , m_projectView(new ProjectView(m_splitter))
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
    UserSettings::instance().setSplitterState(m_splitter->saveState());
}


void MainWindow::SetupMainMenu( )
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *newProjectAction = fileMenu->addAction(tr("&New..."));
    newProjectAction->setShortcut(QKeySequence::New);
    connect(newProjectAction, &QAction::triggered,
            m_projectView, &ProjectView::promptToCreateProject);

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

    connect(m_projectView, &ProjectView::openLayoutRequested,
            this, [this] {
                statusBar()->showMessage(tr("Opening layouts is not available yet"), 3000);
            });
    connect(m_projectView, &ProjectView::openLastLayoutRequested,
            this, [this] {
                statusBar()->showMessage(tr("There is no saved layout to open yet"), 3000);
            });
}


void MainWindow::CreateSplitterView( )
{
    // left side: layout elements view
    LayoutElementsView *layoutElements = new LayoutElementsView();
    layoutElements->addLayoutElements( );
    m_splitter->addWidget( layoutElements );

    // right side: project view
    m_splitter->addWidget(m_projectView);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(8);
    m_splitter->setStyleSheet(
        "QSplitter::handle { background-color: #707070; }"
        "QSplitter::handle:hover { background-color: #3d8ec9; }"
        );

    m_splitter->setSizes({500, 500});

    // restore splitter state from preferences if available
    const QByteArray savedState = UserSettings::instance().splitterState();
    if (!savedState.isEmpty()) {
        m_splitter->restoreState(savedState);
    }

    setCentralWidget(m_splitter);
}


void MainWindow::defineTableOutline()
{
    if (m_projectView->projectTitle().isEmpty()) {
        statusBar()->showMessage(tr("Please load or create a new layout before defining its table"));
        return;
    }

    m_projectView->defineTableOutline();
}


void MainWindow::showCityLayout()
{
    m_projectView->showCityLayout();
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
