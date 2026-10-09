
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

