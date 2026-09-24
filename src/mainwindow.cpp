#include "mainwindow.h"

#include "CityLayoutView.h"
#include "LayoutElementsView.h"
#include "Preferences.h"

#include <QAction>
#include <QInputDialog>
#include <QKeySequence>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QSplitter>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_splitter(new QSplitter(Qt::Horizontal, this))
    , m_cityLayoutView(new CityLayoutView(m_splitter))
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QAction *newProjectAction = fileMenu->addAction(tr("&New..."));
    newProjectAction->setShortcut(QKeySequence::New);
    connect(newProjectAction, &QAction::triggered,
            this, &MainWindow::createNewLayout);

    QMenu *tableMenu = menuBar()->addMenu(tr("&Table"));
    QAction *defineTableAction = tableMenu->addAction(tr("Define Table Outline..."));
    connect(defineTableAction, &QAction::triggered,
            this, &MainWindow::defineTableOutline);
    connect(m_cityLayoutView, &CityLayoutView::createNewLayoutRequested,
            this, &MainWindow::createNewLayout);
    connect(m_cityLayoutView, &CityLayoutView::openLayoutRequested,
            this, [this] {
                statusBar()->showMessage(tr("Opening layouts is not available yet"), 3000);
            });
    connect(m_cityLayoutView, &CityLayoutView::openLastLayoutRequested,
            this, [this] {
                statusBar()->showMessage(tr("There is no saved layout to open yet"), 3000);
            });

    LayoutElementsView *layoutElements = new LayoutElementsView();
    layoutElements->addLayoutElements( );
    m_splitter->addWidget( layoutElements );

    m_splitter->addWidget(m_cityLayoutView);
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
    statusBar()->showMessage(tr("Drag the divider to resize the panels"));
}

MainWindow::~MainWindow()
{
    Preferences::instance().setSplitterState(m_splitter->saveState());
}


void MainWindow::createNewLayout()
{
    bool accepted = false;
    const QString title = QInputDialog::getText(
        this,
        tr("New Layout"),
        tr("Layout title:"),
        QLineEdit::Normal,
        QString(),
        &accepted).trimmed();

    if (!accepted || title.isEmpty()) {
        return;
    }

    m_cityLayoutView->startProject(title);
    setWindowTitle(tr("%1 - LegoCityLayout").arg(title));
    defineTableOutline();
}


void MainWindow::defineTableOutline()
{
    if (m_cityLayoutView->projectTitle().isEmpty()) {
        statusBar()->showMessage(tr("Create a layout before defining its table"), 3000);
        return;
    }

    m_cityLayoutView->beginTableDefinition();
    statusBar()->showMessage(
        tr("Press and drag from an endpoint to draw a side. Release on the starting point to close; Esc cancels and Delete undoes."));
}
