
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

#include <QtMath>
#include "UserSettings.h"
#include "LegoGrid.h"
#include <QActionGroup>
#include <QMenu>
#include <QIcon>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QLabel>
#include "ProjectView.h"

#include "LoadedProjects.h"
#include "CityLayoutView.h"
#include "elements/CityLayoutElement.h"
#include "elements/ElementCatalog.h"
#include "OpenOrCreateProjectView.h"
#include "TableDefinitionEditorView.h"

#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QFile>
#include <QStringList>
#include <QTransform>
#include <cmath>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>


ProjectView::ProjectView(QWidget *parent)
    : QWidget(parent)
    , _Projects(LoadedProjects::instance())
    , m_cityLayoutView(new CityLayoutView(this))
    , m_tableDefinitionEditor(new TableDefinitionEditorView(this))
    , m_openOrCreateProjectView(new OpenOrCreateProjectView(this))

{
    BuildUI( );
}


ProjectView::~ProjectView()
{
    // LoadedProjects deletes element widgets before their parent view is destroyed.
    _Projects.clear();
    delete m_tableDefinitionEditor;
    delete m_cityLayoutView;
}


void ProjectView::BuildUI( )
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    AddToolBar( layout );

    m_cityLayoutHeader = new QWidget(this);
    m_cityLayoutHeader->setObjectName(QStringLiteral("cityLayoutHeader"));
    m_cityLayoutHeader->setAutoFillBackground(true);
    auto *headerLayout = new QVBoxLayout(m_cityLayoutHeader);
    headerLayout->setContentsMargins(14, 10, 14, 10);
    auto *titleLabel = m_cityLayoutTitle = new QLabel(tr("City Layout"), m_cityLayoutHeader);
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setStyleSheet(QStringLiteral("font-size: 22px; font-weight: 700;"));
    headerLayout->addWidget(titleLabel);
    layout->addWidget(m_cityLayoutHeader);

    AddViews( layout );
}


void ProjectView::AddToolBar( QVBoxLayout *layout )
{
    m_toolbar = new QToolBar(tr("Project tools"), this);
    m_toolbar->setObjectName(QStringLiteral("projectToolbar"));
    m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_toolbar->setIconSize(QSize(24, 24));
    m_toolbar->setMovable(false);
    m_toolbar->setStyleSheet(QStringLiteral(
        "QToolBar#projectToolbar QToolButton:checked {"
        " background-color: #c8e6c9;"
        " border: 1px solid #81c784;"
        " border-radius: 4px;"
        " }"));

    auto *viewGroup = new QActionGroup(m_toolbar);
    m_tableViewAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/table.svg")), tr("Table View"));
    m_tableViewAction->setCheckable(true);
    viewGroup->addAction(m_tableViewAction);
    m_cityViewAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/city.svg")), tr("City View"));
    m_cityViewAction->setCheckable(true);
    viewGroup->addAction(m_cityViewAction);
    connect(m_tableViewAction, &QAction::triggered, this, &ProjectView::defineTableOutline);
    connect(m_cityViewAction, &QAction::triggered, this, &ProjectView::showCityLayout);
    m_toolbar->addSeparator();
    m_unitsAction = m_toolbar->addAction(tr("Units"));
    auto *unitsMenu = new QMenu(m_toolbar);
    auto *unitsGroup = new QActionGroup(unitsMenu);
    for (auto system : {UserSettings::MeasurementSystem::Imperial,
                        UserSettings::MeasurementSystem::Metric,
                        UserSettings::MeasurementSystem::Studs,
                        UserSettings::MeasurementSystem::Plates}) {
        QString name;
        switch (system) {
        case UserSettings::MeasurementSystem::Metric: name = tr("Centimeters"); break;
        case UserSettings::MeasurementSystem::Studs: name = tr("Studs"); break;
        case UserSettings::MeasurementSystem::Plates: name = tr("Plates"); break;
        default: name = tr("Inches"); break;
        }
        auto *action = unitsMenu->addAction(name);
        action->setCheckable(true);
        unitsGroup->addAction(action);
        action->setData(static_cast<int>(system));
        connect(action, &QAction::triggered, this, [this, system] {
            UserSettings::instance().setMeasurementSystem(system);
            m_tableDefinitionEditor->refreshMeasurementUnits();
            m_cityLayoutView->update();
            refreshToolbar();
        });
    }
    connect(unitsMenu, &QMenu::aboutToShow, this, [unitsGroup] {
        for (auto *action : unitsGroup->actions())
            action->setChecked(action->data().toInt() == static_cast<int>(UserSettings::instance().measurementSystem()));
    });

    auto *unitsButton = qobject_cast<QToolButton *>(m_toolbar->widgetForAction(m_unitsAction));
    unitsButton->setMenu(unitsMenu);
    unitsButton->setPopupMode(QToolButton::InstantPopup);
    m_gridAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/grid.svg")), tr("Show Grid"));
    m_gridAction->setCheckable(true);
    m_snapAction = m_toolbar->addAction(QIcon(QStringLiteral(":/icons/grid-highlight.svg")), tr("Snap to Grid"));
    m_snapAction->setCheckable(true);
    connect(m_gridAction, &QAction::toggled, this, [this](bool checked) {
        if (!_Projects.currentProject()) return;
        _Projects.tableEditor().gridVisible = checked;
        m_tableDefinitionEditor->update();
        m_cityLayoutView->update();
        refreshToolbar();
    });

    connect(m_snapAction, &QAction::toggled, this, [this](bool checked) {
        if (!_Projects.currentProject()) return;
        _Projects.tableEditor().snapToGrid = checked;
        m_tableDefinitionEditor->refreshMeasurementUnits();
        m_cityLayoutView->update();
    });

    layout->addWidget(m_toolbar);
}


void ProjectView::AddViews( QVBoxLayout *layout )
{
    // add stacked views
    m_views = new QStackedWidget(this);
    layout->addWidget(m_views);

    // Open / Create View
    m_views->addWidget(m_openOrCreateProjectView);
    connect(m_openOrCreateProjectView,
            &OpenOrCreateProjectView::projectTitleAccepted,
            this, &ProjectView::initializeNewProject);
    connect(m_openOrCreateProjectView,
            &OpenOrCreateProjectView::openLayoutRequested,
            this, &ProjectView::openLayoutRequested);

    connect(m_openOrCreateProjectView,
            &OpenOrCreateProjectView::openLastLayoutRequested,
            this, &ProjectView::openLastLayoutRequested);

    // City View
    m_views->addWidget(m_cityLayoutView);

    // Table Defintion View
    m_views->addWidget(m_tableDefinitionEditor);        
    connect(m_tableDefinitionEditor, &TableDefinitionEditorView::measurementUnitsChanged, this, &ProjectView::refreshToolbar);
    connect(m_tableDefinitionEditor, &TableDefinitionEditorView::editingFinished, this, &ProjectView::showCityLayout);

    setCurrentWidget(m_openOrCreateProjectView);
}


void ProjectView::promptToCreateProject()
{
    m_openOrCreateProjectView->createNewLayout();
}


void ProjectView::initializeNewProject(const QString &title)
{
    startProject(title);
    defineTableOutline();

    auto *mainWindow = qobject_cast<QMainWindow *>(window());
    if (mainWindow)
        mainWindow->setWindowTitle(tr("Lego City Layout"));
}


void ProjectView::startProject(const QString &title)
{
    const int index = _Projects.addProject(title);
    if (index < 0) return;
    _Projects.setCurrentProjectIndex(index);
    _Projects.currentProject()->savedTableState = tableState();
}


bool ProjectView::setCurrentProjectIndex(int index)
{
    if (index != -1 && !_Projects.project(index))
        return false;

    // hide the current project's elements
    if (auto *previousProject = _Projects.currentProject()) {
        for (auto *element : previousProject->cityLayouts) element->hide();
    }

    // switch to the new project
    _Projects.setCurrentProjectIndex(index);
    if (index == -1) {
        m_cityLayoutView->activateProject();
        setCurrentWidget(m_openOrCreateProjectView);
        return true;
    }

    for (auto *element : _Projects.cityLayouts())
        element->show();

    m_cityLayoutView->activateProject();
    m_tableDefinitionEditor->activateProject();

    setCurrentWidget(_Projects.currentProject()->editingTable
                         ? static_cast<QWidget *>(m_tableDefinitionEditor)
                         : static_cast<QWidget *>(m_cityLayoutView));

    currentWidget()->setFocus(Qt::OtherFocusReason);
    currentWidget()->update();

    if (auto *mainWindow = qobject_cast<QMainWindow *>(window())) {
        mainWindow->setWindowTitle(tr("Lego City Layout"));
        mainWindow->statusBar()->clearMessage();
    }

    return true;
}


void ProjectView::defineTableOutline()
{
    if (auto *project = _Projects.currentProject()) {
        if (!project->editingTable) m_tableDefinitionEditor->begin();
        project->editingTable = true;
        setCurrentWidget(m_tableDefinitionEditor);
    }
}

void ProjectView::showCityLayout()
{
    if (auto *project = _Projects.currentProject()) {
        project->editingTable = false;
        setCurrentWidget(m_cityLayoutView);
        m_cityLayoutView->setFocus(Qt::OtherFocusReason);
        m_cityLayoutView->update();
    } else {
        setCurrentWidget(m_openOrCreateProjectView);
    }
}


bool ProjectView::saveTableDefinition()
{
    if (_Projects.title().isEmpty()) {
        QMessageBox::information(this, tr("Save Table Definition"),
                                 tr("Create a layout before saving its table definition."));
        return false;
    }
    const QString existingPath = _Projects.currentProject()->filePath;
    const QString directory = existingPath.isEmpty() ? UserSettings::instance().layoutSaveDirectory()
        : QFileInfo(existingPath).absolutePath();
    if (directory.isEmpty() || !QDir(directory).exists()) return chooseLayoutFilename();
    return writeLayout(QDir(directory).filePath(projectFilename()));
}

QString ProjectView::projectFilename() const
{
    QString filename = _Projects.title().trimmed();
    filename.replace(QRegularExpression(QStringLiteral("[<>:\\x22/\\\\|?*\\x00-\\x1f]")), QStringLiteral("_"));
    filename.remove(QRegularExpression(QStringLiteral("[. ]+$")));
    if (filename.isEmpty()) {
        filename = QStringLiteral("Layout");
    }
    if (!filename.endsWith(QStringLiteral(".LegoLayout"), Qt::CaseInsensitive))
        filename += QStringLiteral(".LegoLayout");
    return filename;
}

bool ProjectView::chooseLayoutFilename()
{
    const QString remembered = UserSettings::instance().layoutSaveDirectory();
    const QString directory = QDir(remembered).exists() && !remembered.isEmpty() ? remembered
        : QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QFileDialog dialog(this, tr("Save Lego Layout"));
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setOption(QFileDialog::DontConfirmOverwrite, true);
    dialog.setFileMode(QFileDialog::AnyFile);
    dialog.setNameFilter(tr("Lego Layouts (*.LegoLayout)"));
    dialog.setDefaultSuffix(QStringLiteral("LegoLayout"));
    dialog.setDirectory(directory);
    dialog.selectFile(projectFilename());
    if (dialog.exec() != QDialog::Accepted || dialog.selectedFiles().isEmpty()) {
        return false;
    }
    const QString path = dialog.selectedFiles().first();
    const QString originalTitle = _Projects.title();
    // An explicitly edited filename becomes the project name for subsequent saves.
    if (QFileInfo(path).fileName() != projectFilename())
        _Projects.currentProject()->setTitle(QFileInfo(path).completeBaseName());
    if (writeLayout(path)) return true;
    _Projects.currentProject()->setTitle(originalTitle);
    return false;
}

bool ProjectView::writeLayout(const QString &path)
{
    const QString projectTitle = _Projects.title();
    QSettings settings;
    const auto pointJson = [](const QPointF &point) {
        return QJsonObject{{QStringLiteral("x"), point.x()},
                           {QStringLiteral("y"), point.y()}};
    };


    QJsonArray surfaces;
    for (const TableSurface &surface : _Projects.currentProject()->tableDefinition.surfaces()) {
        QJsonArray outline;
        for (const QPointF &point : surface.outline()) {
            outline.append(pointJson(point));
        }
        surfaces.append(QJsonObject{{QStringLiteral("name"), surface.name()},
                                    {QStringLiteral("outline"), outline}});
    }
    QJsonArray openSides;
    for (const QLineF &side : _Projects.currentProject()->tableDefinition.openSides()) {
        openSides.append(QJsonObject{{QStringLiteral("start"), pointJson(side.p1())},
                                     {QStringLiteral("end"), pointJson(side.p2())}});
    }
    // for (const auto *element : _Projects.cityLayouts()) {
    //     if (!ElementCatalog::instance().byResource(element->resourcePath())) {
    //         QMessageBox::critical(this, tr("Could Not Save Layout"),
    //                               tr("Unknown element type: %1").arg(element->name()));
    //         return false;
    //     }
    // }

    const QJsonDocument document(QJsonObject{
        {QStringLiteral("format"), QStringLiteral("LegoCityLayout.TableDefinition")},
        {QStringLiteral("version"), 2},
        {QStringLiteral("layoutName"), projectTitle},
        {QStringLiteral("coordinateUnits"), QStringLiteral("canvas")},
        {QStringLiteral("surfaces"), surfaces},
        {QStringLiteral("openSides"), openSides},
        {QStringLiteral("cityLayout"), m_cityLayoutView->savedLayout()}});
    const QByteArray data = document.toJson(QJsonDocument::Indented);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size() || !file.commit()) {
        QMessageBox::critical(this, tr("Could Not Save Table Definition"),
                              tr("Could not save %1:\n%2")
                                  .arg(QDir::toNativeSeparators(path), file.errorString()));
        return false;
    }
    _Projects.currentProject()->filePath = QFileInfo(path).absoluteFilePath();
    _Projects.currentProject()->savedTableState = tableState();
    _Projects.currentProject()->savedLayoutRevision = m_cityLayoutView->changeRevision();
    UserSettings::instance().setLayoutSaveDirectory(QFileInfo(path).absolutePath());
    settings.setValue(QStringLiteral("layout/lastPath"), QFileInfo(path).absoluteFilePath());
    if (auto *mainWindow = qobject_cast<QMainWindow *>(window())) {
        mainWindow->statusBar()->showMessage(
            tr("Table definition saved to %1").arg(QDir::toNativeSeparators(path)), 5000);
    }
    return true;
}


void ProjectView::promptToOpenLayout()
{
    QSettings settings;
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open Layout"),
        settings.value(QStringLiteral("layout/lastPath"),
                       QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString(),
        tr("Lego Layouts (*.LegoLayout)"));
    if (!path.isEmpty()) openLayout(path);
}

void ProjectView::openLastLayout()
{
    const QString path = QSettings().value(QStringLiteral("layout/lastPath")).toString();
    if (path.isEmpty()) {
        QMessageBox::information(this, tr("Open Last Layout"), tr("Open or save a layout first."));
        return;
    }
    openLayout(path);
}

bool ProjectView::openLayout(const QString &path)
{
    const auto fail = [this, &path](const QString &reason) {
        QMessageBox::critical(this, tr("Could Not Open Layout"),
                              tr("Could not open %1:\n%2").arg(QDir::toNativeSeparators(path), reason));
        return false;
    };
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return fail(file.errorString());
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) return fail(error.errorString());
    const QJsonObject root = document.object();
    if (root.value("format").toString() != QStringLiteral("LegoCityLayout.TableDefinition")
        || root.value("version").toInt() != 2
        || root.value("layoutName").toString().trimmed().isEmpty()
        || !root.value("surfaces").isArray() || !root.value("openSides").isArray())
        return fail(tr("Unsupported or invalid layout file."));

    const auto readPoint = [](const QJsonValue &value, QPointF &point) {
        const auto object = value.toObject();
        if (!object.value("x").isDouble() || !object.value("y").isDouble()) return false;
        point = QPointF(object.value("x").toDouble(), object.value("y").toDouble());
        return std::isfinite(point.x()) && std::isfinite(point.y());
    };
    TableDefinition table;
    for (const auto &value : root.value("surfaces").toArray()) {
        const auto surface = value.toObject();
        if (!surface.value("outline").isArray()) return fail(tr("Invalid table outline."));
        QPolygonF outline;
        for (const auto &vertex : surface.value("outline").toArray()) {
            QPointF point;
            if (!readPoint(vertex, point)) return fail(tr("Invalid table coordinates."));
            outline.append(point);
        }
        if (!table.addSurface(TableSurface(surface.value("name").toString(), outline)))
            return fail(tr("Invalid table surface."));
    }
    QList<QLineF> sides;
    for (const auto &value : root.value("openSides").toArray()) {
        QPointF start, end;
        if (!readPoint(value.toObject().value("start"), start)
            || !readPoint(value.toObject().value("end"), end))
            return fail(tr("Invalid table side."));
        sides.append(QLineF(start, end));
    }
    table.setOpenSides(sides);

    // Older table files have no cityLayout section.
    const auto city = root.value("cityLayout").toObject();
    const double zoom = city.value("zoomFactor").toDouble(1.875);
    const QPointF origin(city.value("gridOriginX").toDouble(), city.value("gridOriginY").toDouble());
    if (!std::isfinite(zoom) || zoom < 0.25 || zoom > 24.0
        || !std::isfinite(origin.x()) || !std::isfinite(origin.y())
        || (root.contains("cityLayout") && (!root.value("cityLayout").isObject()
            || !city.value("elements").isArray())))
        return fail(tr("Invalid city layout."));

    struct Element { QString name; QString resourcePath; QPixmap image; QSize size; QPointF position; qreal rotation; qreal flexBend; };
    QList<Element> elementList;
    QStringList missingTypes;
    int missingCount = 0;
    for (const auto &value : city.value("elements").toArray())
    {
        QJsonObject jsonElement = value.toObject();
        Element element;
        element.name = jsonElement.value("name").toString();
        if (element.name.trimmed().isEmpty()) return fail(tr("Invalid city element."));
        const auto *definition = ElementCatalog::instance().byName(element.name);
        if (!definition) {
            ++missingCount;
            if (!missingTypes.contains(element.name)) missingTypes.append(element.name);
            continue;
        }
        element.name = definition->displayName;
        element.resourcePath = definition->resourcePath;
        element.size = definition->size;
        element.image.load(element.resourcePath);
        QPointF position;
        if (!readPoint(value, position) || qAbs(position.x()) > 10000000 || qAbs(position.y()) > 10000000)
            return fail(tr("Invalid plate position."));
        element.position = position;
        element.rotation = jsonElement.value("rotationDegrees").toDouble(-1);

        if (jsonElement.contains("flexBendDegrees") && !jsonElement.value("flexBendDegrees").isDouble())
            return fail(tr("Invalid flex track bend."));

        element.flexBend = jsonElement.value("flexBendDegrees").toDouble(0.0);

        if (!std::isfinite(element.rotation) || element.rotation < 0 || element.rotation >= 360 || element.image.isNull())
            return fail(tr("Invalid city element."));

        const CityLayoutElement identity(element.name, element.image, element.size,
                                         LegoGrid::pixelsPerStud, 1.0, nullptr, element.resourcePath);

        if (!std::isfinite(element.flexBend) || qAbs(element.flexBend) > LegoGrid::flexMaximumBendDegrees
            || (!identity.isFlexTrack() && !qFuzzyIsNull(element.flexBend)))
            return fail(tr("Invalid flex track bend."));

        // Track connections can align pieces between the manual rotation steps.
        // Preserve those saved angles; only baseplates require quarter turns.
        if (!identity.isTrack()) {
            const qreal rotationStep = identity.rotationStepDegrees();
            const qreal rotationOffset = qAbs(element.rotation / rotationStep - qRound(element.rotation / rotationStep));
            if (rotationOffset > 1e-9)
                return fail(tr("Invalid city plate."));
        }

        elementList.append(element);
    }

    // Validate available elements before asking, and commit no project state
    // until the user has accepted opening without every missing placement.
    if (missingCount > 0) {
        const auto choice = QMessageBox::warning(
            this, tr("Missing Elements"),
            tr("%n saved element(s) are missing from the element catalog:", nullptr, missingCount)
                + QStringLiteral("\n\n") + missingTypes.join(QStringLiteral("\n"))
                + tr("\n\nIf you save this layout, the missing elements will be omitted."
                     "\n\nContinue opening this layout without these elements?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (choice != QMessageBox::Yes) return false;
    }

    const int index = _Projects.addProject(root.value("layoutName").toString());
    auto *project = _Projects.project(index);
    project->tableDefinition = table;
    project->zoomFactor = zoom;
    project->tableEditor.viewScale = zoom;
    project->gridOrigin = origin;
    for (const auto &plate : elementList) {
        auto *element = new CityLayoutElement(plate.name, plate.image, plate.size, LegoGrid::pixelsPerStud, zoom, m_cityLayoutView, plate.resourcePath);
        element->setFlexBendDegrees(plate.flexBend);
        element->rotateByDegrees(plate.rotation);
        element->setLayoutPosition(plate.position);
        project->cityLayouts.append(element);
    }
    setCurrentProjectIndex(index);
    project->filePath = QFileInfo(path).absoluteFilePath();
    project->savedTableState = tableState();
    project->savedLayoutRevision = m_cityLayoutView->changeRevision();
    QSettings settings;
    settings.setValue(QStringLiteral("layout/lastPath"), QFileInfo(path).absoluteFilePath());
    return true;
}


bool ProjectView::confirmSaveCurrentProject()
{
    QString projectTitle = _Projects.title();
    if (!projectTitle.isEmpty()
        && (tableState() != _Projects.currentProject()->savedTableState
            || m_cityLayoutView->changeRevision() != _Projects.currentProject()->savedLayoutRevision)) {
        const auto choice = QMessageBox::warning(
            this, tr("Save Changes?"),
            tr("Save changes to \"%1\" before closing?").arg(projectTitle),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);
        if (choice == QMessageBox::Cancel) return false;
        if (choice == QMessageBox::Save && !saveTableDefinition()) return false;
    }

    return true;
}

bool ProjectView::confirmCloseAllProjects()
{
    const int originalIndex = _Projects.currentProjectIndex();
    for (int index = 0; index < _Projects.projectCount(); ++index) {
        setCurrentProjectIndex(index);
        if (!confirmSaveCurrentProject()) {
            setCurrentProjectIndex(originalIndex);
            return false;
        }
    }
    setCurrentProjectIndex(originalIndex);
    return true;
}

void ProjectView::closeProject()
{
    if (!confirmSaveCurrentProject()) return;

    _Projects.removeProject(_Projects.currentProjectIndex());
    _Projects.setCurrentProjectIndex(-1);
    setCurrentWidget(m_openOrCreateProjectView);
    m_openOrCreateProjectView->setFocus(Qt::OtherFocusReason);
    if (auto *mainWindow = qobject_cast<QMainWindow *>(window())) {
        mainWindow->setWindowTitle(tr("Lego City Layout"));
        mainWindow->statusBar()->clearMessage();
    }
}


QByteArray ProjectView::tableState()
{
    if (!_Projects.currentProject()) {
        return {};
    }
    const auto pointJson = [this](const QPointF &point) {
        const QPointF position = m_cityLayoutView->projectPoint(point);
        // Ignore floating point noise introduced by view zooming.
        return QJsonArray{qRound64(position.x() * 1000000.0),
                          qRound64(position.y() * 1000000.0)};
    };
    QJsonArray surfaces;
    for (const TableSurface &surface : _Projects.currentProject()->tableDefinition.surfaces()) {
        QJsonArray outline;
        for (const QPointF &point : surface.outline()) {
            outline.append(pointJson(point));
        }
        surfaces.append(QJsonObject{{QStringLiteral("name"), surface.name()},
                                    {QStringLiteral("outline"), outline}});
    }
    QJsonArray sides;
    for (const QLineF &side : _Projects.currentProject()->tableDefinition.openSides()) {
        sides.append(QJsonArray{pointJson(side.p1()), pointJson(side.p2())});
    }
    return QJsonDocument(QJsonObject{{QStringLiteral("surfaces"), surfaces},
                                    {QStringLiteral("openSides"), sides}})
        .toJson(QJsonDocument::Compact);
}

void ProjectView::setCurrentWidget(QWidget *widget)
{
    m_views->setCurrentWidget(widget);
    refreshToolbar();
}

QWidget *ProjectView::currentWidget() const
{
    return m_views->currentWidget();
}

void ProjectView::refreshToolbar()
{
    const bool loaded = _Projects.currentProject() != nullptr;
    m_cityLayoutTitle->setText(loaded ? tr("City Layout - %1").arg(_Projects.title()) : tr("City Layout"));
    m_cityLayoutHeader->setVisible(loaded && currentWidget() == m_cityLayoutView);
    m_toolbar->setVisible(loaded && currentWidget() != m_openOrCreateProjectView);
    m_tableViewAction->setEnabled(loaded);
    m_cityViewAction->setEnabled(loaded);
    m_tableViewAction->setChecked(loaded && currentWidget() == m_tableDefinitionEditor);
    m_cityViewAction->setChecked(loaded && currentWidget() == m_cityLayoutView);
    m_gridAction->setEnabled(loaded);
    m_snapAction->setEnabled(loaded);
    const QSignalBlocker gridBlocker(m_gridAction);
    const QSignalBlocker snapBlocker(m_snapAction);
    m_gridAction->setChecked(loaded && _Projects.tableEditor().gridVisible);
    m_snapAction->setChecked(loaded && _Projects.tableEditor().snapToGrid);
    m_gridAction->setToolTip(m_gridAction->isChecked() ? tr("Hide Grid") : tr("Show Grid"));
    const auto &settings = UserSettings::instance();
    m_unitsAction->setIcon(QIcon(QStringLiteral(":/icons/units-%1.svg")
                                   .arg(settings.measurementAbbreviation())));
    m_unitsAction->setToolTip(tr("Units: %1").arg(settings.measurementName()));
}
