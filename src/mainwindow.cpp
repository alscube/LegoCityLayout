#include "mainwindow.h"

#include "LayoutElementsView.h"
#include "Preferences.h"
#include "ProjectView.h"

#include <QAction>
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
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *newProjectAction = fileMenu->addAction(tr("&New..."));
    newProjectAction->setShortcut(QKeySequence::New);
    connect(newProjectAction, &QAction::triggered,
            m_projectView, &ProjectView::promptToCreateProject);

    QMenu *tableMenu = menuBar()->addMenu(tr("&Table"));

    QAction *defineTableAction = tableMenu->addAction(tr("Define Table Outline..."));
    connect(defineTableAction, &QAction::triggered,
            this, &MainWindow::defineTableOutline);

    QAction *showLayoutAction = tableMenu->addAction(tr("Show City Layout"));
    connect(showLayoutAction, &QAction::triggered, this, &MainWindow::showCityLayout);

    // connect(m_projectView, &ProjectView::projectStarted,
    //         this, [this](const QString &title) {
    //             setWindowTitle(tr("%1 - LegoCityLayout").arg(title));
    //             statusBar()->showMessage(
    //                 tr("Press and drag from an endpoint to draw a side. Release on the starting point to close; Esc cancels and Delete undoes."));
    //         });

    connect(m_projectView, &ProjectView::openLayoutRequested,
            this, [this] {
                statusBar()->showMessage(tr("Opening layouts is not available yet"), 3000);
            });
    connect(m_projectView, &ProjectView::openLastLayoutRequested,
            this, [this] {
                statusBar()->showMessage(tr("There is no saved layout to open yet"), 3000);
            });

    LayoutElementsView *layoutElements = new LayoutElementsView();
    layoutElements->addLayoutElements( );
    m_splitter->addWidget( layoutElements );

    m_splitter->addWidget(m_projectView);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(8);
    m_splitter->setStyleSheet(
        "QSplitter::handle { background-color: #707070; }"
        "QSplitter::handle:hover { background-color: #3d8ec9; }"
    );
    m_splitter->setSizes({500, 500});

    const QByteArray savedState = Preferences::instance().splitterState();
    if (!savedState.isEmpty()) {
        m_splitter->restoreState(savedState);
    }

    setCentralWidget(m_splitter);
    setWindowTitle(tr("Lego City Layout"));
    resize(1000, 700);
}


MainWindow::~MainWindow()
{
    Preferences::instance().setSplitterState(m_splitter->saveState());
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
