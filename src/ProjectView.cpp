
#include <QtMath>
#include "UserSettings.h"
#include <QActionGroup>
#include <QMenu>
#include <QIcon>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include "ProjectView.h"

#include "LoadedProjects.h"
#include "CityLayoutView.h"
#include "CityLayoutElement.h"
#include "OpenOrCreateProjectView.h"
#include "TableDefinitionEditorView.h"

#include <QMainWindow>
#include <QDir>
#include <QFileDialog>
#include <QFile>
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
        case UserSettings::MeasurementSystem::Metric: name = tr("Millimeters"); break;
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
        mainWindow->setWindowTitle( tr("Lego City Layout - %1").arg(title) );
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
    if (!_Projects.project(index))
        return false;

    // hide the current project's elements
    if (auto *previousProject = _Projects.currentProject()) {
        for (auto *element : previousProject->cityLayouts) element->hide();
    }

    // switch to the new project
    _Projects.setCurrentProjectIndex(index);

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
        mainWindow->setWindowTitle(tr("Lego City Layout - %1").arg(LoadedProjects::instance().title()));
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
    QString projectTitle = _Projects.title();
    if (projectTitle.isEmpty()) {
        QMessageBox::information(this, tr("Save Table Definition"),
                                 tr("Create a layout before saving its table definition."));
        return false;
    }

    QSettings settings;
    const QString directory = QFileDialog::getExistingDirectory(
        this, tr("Choose Table Definition Directory"),
        settings.value(QStringLiteral("tableDefinition/saveDirectory"),
                       QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)).toString());
    if (directory.isEmpty()) {
        return false;
    }

    QString filename = projectTitle.trimmed();
    filename.replace(QRegularExpression(QStringLiteral("[<>:\\x22/\\\\|?*\\x00-\\x1f]")), QStringLiteral("_"));
    filename.remove(QRegularExpression(QStringLiteral("[. ]+$")));
    if (filename.isEmpty()) {
        filename = QStringLiteral("Layout");
    }
    const QString path = QDir(directory).filePath(filename + QStringLiteral(".table.json"));
    if (QFileInfo::exists(path)
        && QMessageBox::question(this, tr("Replace Table Definition?"),
                                 tr("%1 already exists. Replace it?").arg(QDir::toNativeSeparators(path)),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) {
        return false;
    }

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
    const QJsonDocument document(QJsonObject{
        {QStringLiteral("format"), QStringLiteral("LegoCityLayout.TableDefinition")},
        {QStringLiteral("version"), 1},
        {QStringLiteral("layoutName"), projectTitle},
        {QStringLiteral("coordinateUnits"), QStringLiteral("canvas")},
        {QStringLiteral("gridSpacing"), 64.0},
        {QStringLiteral("gridSizeInches"), 10.0},
        {QStringLiteral("gridSizeCentimeters"), 25.5},
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
    _Projects.currentProject()->savedTableState = tableState();
    _Projects.currentProject()->savedLayoutRevision = m_cityLayoutView->changeRevision();
    settings.setValue(QStringLiteral("tableDefinition/saveDirectory"), directory);
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
        tr("Lego Layouts (*.table.json);;JSON Files (*.json)"));
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
        || root.value("version").toInt() != 1
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
    struct Plate { QString name; QPixmap image; QSize size; QPoint position; int rotation; };
    QList<Plate> plates;
    for (const auto &value : city.value("elements").toArray()) {
        const auto element = value.toObject();
        Plate plate;
        plate.name = element.value("name").toString();
        plate.size = QSize(element.value("widthStuds").toInt(), element.value("heightStuds").toInt());
        QPointF position;
        if (!readPoint(value, position) || qAbs(position.x()) > 10000000 || qAbs(position.y()) > 10000000)
            return fail(tr("Invalid plate position."));
        plate.position = position.toPoint();
        plate.rotation = element.value("rotationDegrees").toInt(-1);
        if (plate.size.width() <= 0 || plate.size.height() <= 0
            || plate.rotation < 0 || plate.rotation >= 360 || plate.rotation % 90 != 0
            || !plate.image.loadFromData(QByteArray::fromBase64(
                element.value("imagePngBase64").toString().toLatin1()), "PNG"))
            return fail(tr("Invalid city plate."));
        // Saved PNGs already include the plate's rotation.
        plate.image = plate.image.transformed(QTransform().rotate(-plate.rotation));
        plates.append(plate);
    }

    const int index = _Projects.addProject(root.value("layoutName").toString());
    auto *project = _Projects.project(index);
    project->tableDefinition = table;
    project->zoomFactor = zoom;
    project->tableEditor.viewScale = zoom;
    project->gridOrigin = origin;
    for (const auto &plate : plates) {
        auto *element = new CityLayoutElement(plate.name, plate.image, plate.size, 2.0, zoom, m_cityLayoutView);
        element->rotateQuarterTurns(plate.rotation / 90);
        element->move(plate.position);
        project->cityLayouts.append(element);
    }
    setCurrentProjectIndex(index);
    project->savedTableState = tableState();
    project->savedLayoutRevision = m_cityLayoutView->changeRevision();
    QSettings settings;
    settings.setValue(QStringLiteral("layout/lastPath"), QFileInfo(path).absoluteFilePath());
    return true;
}


void ProjectView::closeProject()
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
        if (choice == QMessageBox::Cancel
            || (choice == QMessageBox::Save && !saveTableDefinition())) {
            return;
        }
    }

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
