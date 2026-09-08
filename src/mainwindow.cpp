#include "mainwindow.h"

#include "CityLayoutView.h"
#include "LayoutElementsView.h"
#include "Preferences.h"

#include <QSplitter>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_splitter(new QSplitter(Qt::Horizontal, this))
{
    LayoutElementsView *layoutElements = new LayoutElementsView();
    layoutElements->addLayoutElements( );
    m_splitter->addWidget( layoutElements );

    m_splitter->addWidget(new CityLayoutView(m_splitter));
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
    setWindowTitle(tr("LegoCityLayout"));
    resize(1000, 700);
    statusBar()->showMessage(tr("Drag the divider to resize the panels"));
}

MainWindow::~MainWindow()
{
    Preferences::instance().setSplitterState(m_splitter->saveState());
}
