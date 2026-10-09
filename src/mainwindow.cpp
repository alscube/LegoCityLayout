
// Copyright 2026. Alan Krzywicki

// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "mainwindow.h"

#include "LoadedProjects.h"
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
    , _Projects(LoadedProjects::instance())
    , _splitter(new QSplitter(Qt::Horizontal, this))
    , _projectView(new ProjectView(_splitter))
{

    // note: ProjectView is needed to setup the Main Menu
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

    auto *openAction = fileMenu->addAction(tr("&Open Layout..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, _projectView, &ProjectView::promptToOpenLayout);
    auto *openLastAction = fileMenu->addAction(tr("Open &Last Layout"));
    connect(openLastAction, &QAction::triggered, _projectView, &ProjectView::openLastLayout);

    fileMenu->addSeparator();
    QAction *closeProjectAction = fileMenu->addAction(tr("&Close Layout..."));
    closeProjectAction->setShortcut(QKeySequence::Close);
    connect(closeProjectAction, &QAction::triggered,
            _projectView, &ProjectView::closeProject);

    QAction *saveTableAction = fileMenu->addAction(tr("&Save Layout..."));
    saveTableAction->setShortcut(QKeySequence::Save);
    connect(saveTableAction, &QAction::triggered,
            _projectView, &ProjectView::saveTableDefinition);

    QMenu *tableMenu = menuBar()->addMenu(tr("&View"));

    QAction *defineTableAction = tableMenu->addAction(tr("Edit &Table Outline..."));
    connect(defineTableAction, &QAction::triggered,
            this, &MainWindow::defineTableOutline);

    QAction *showLayoutAction = tableMenu->addAction(tr("Edit City &Layout"));
    connect(showLayoutAction, &QAction::triggered, this, &MainWindow::showCityLayout);

    QMenu *projectsMenu = menuBar()->addMenu(tr("&Projects"));
    connect(projectsMenu, &QMenu::aboutToShow, this, [this, projectsMenu]
    {
        projectsMenu->clear();
        int projectCount = _Projects.projectCount();
        for (int index = 0; index < projectCount; ++index)
        {
            auto *action = projectsMenu->addAction(_Projects.project(index)->title());
            action->setCheckable(true);
            action->setChecked(index == _Projects.currentProjectIndex());
            connect(action, &QAction::triggered, this, [this, index] {
                _projectView->setCurrentProjectIndex(index);
            });
        }
        if (_Projects.projectCount() == 0) {
            projectsMenu->addAction(tr("No Loaded Projects"))->setEnabled(false);
        }
    });

    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));
    QAction *settingsAction = settingsMenu->addAction(tr("Settings..."));
    settingsAction->setMenuRole(QAction::PreferencesRole);
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);

    connect(_projectView, &ProjectView::openLayoutRequested,
            _projectView, &ProjectView::promptToOpenLayout);
    connect(_projectView, &ProjectView::openLastLayoutRequested,
            _projectView, &ProjectView::openLastLayout);

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
    if (_Projects.title().isEmpty()) {
        statusBar()->showMessage(tr("Please load or create a new project before defining its layout"));
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
