
#include <QtMath>
#include "UserSettings.h"
#include <QActionGroup>
#include <QMenu>
#include <QPainter>
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
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    m_toolbar = new QToolBar(tr("Project tools"), this);
    m_toolbar->setObjectName(QStringLiteral("projectToolbar"));
    m_toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_toolbar->setIconSize(QSize(24, 24));
    m_toolbar->setMovable(false);
    const auto icon = [this](int kind) {
        QPixmap pixmap(24, 24);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(palette().color(QPalette::WindowText), 1.5));
        if (kind == 3) {
            painter.drawRect(4, 5, 16, 11);
            painter.drawLine(6, 16, 6, 21);
            painter.drawLine(18, 16, 18, 21);
        } else if (kind == 4) {
            painter.drawRect(3, 12, 5, 9);
            painter.drawRect(10, 4, 5, 17);
            painter.drawRect(17, 9, 4, 12);
            painter.drawLine(12, 7, 13, 7);
            painter.drawLine(12, 10, 13, 10);
        } else if (kind == 0) {
            painter.drawRect(3, 7, 18, 10);
            for (int x = 6; x < 21; x += 3) painter.drawLine(x, 7, x, x % 2 ? 11 : 14);
        } else {
            for (int x = 4; x <= 20; x += 8) {
                painter.drawLine(x, 4, x, 20);
                painter.drawLine(4, x, 20, x);
            }
            if (kind == 2) {
                painter.setBrush(palette().color(QPalette::Highlight));
                painter.drawEllipse(QPointF(12, 12), 3, 3);
            }
        }
        return QIcon(pixmap);
    };
    auto *viewGroup = new QActionGroup(m_toolbar);
    m_tableViewAction = m_toolbar->addAction(icon(3), tr("Table View"));
    m_tableViewAction->setCheckable(true);
    viewGroup->addAction(m_tableViewAction);
    m_cityViewAction = m_toolbar->addAction(icon(4), tr("City View"));
    m_cityViewAction->setCheckable(true);
    viewGroup->addAction(m_cityViewAction);
    connect(m_tableViewAction, &QAction::triggered, this, &ProjectView::defineTableOutline);
    connect(m_cityViewAction, &QAction::triggered, this, &ProjectView::showCityLayout);
    m_toolbar->addSeparator();
    m_unitsAction = m_toolbar->addAction(icon(0), tr("Units"));
    auto *unitsMenu = new QMenu(m_toolbar);
    auto *unitsGroup = new QActionGroup(unitsMenu);
    for (auto system : {UserSettings::MeasurementSystem::Imperial,
                        UserSettings::MeasurementSystem::Metric}) {
        auto *action = unitsMenu->addAction(system == UserSettings::MeasurementSystem::Metric
                                               ? tr("Centimeters") : tr("Inches"));
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
    m_gridAction = m_toolbar->addAction(icon(1), tr("Show Grid"));
    m_gridAction->setCheckable(true);
    m_snapAction = m_toolbar->addAction(icon(2), tr("Snap to Grid"));
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
    m_views = new QStackedWidget(this);
    layout->addWidget(m_views);
    connect(m_tableDefinitionEditor, &TableDefinitionEditorView::measurementUnitsChanged,
            this, &ProjectView::refreshToolbar);
    m_views->addWidget(m_openOrCreateProjectView);
    m_views->addWidget(m_cityLayoutView);
    m_views->addWidget(m_tableDefinitionEditor);
    connect(m_tableDefinitionEditor, &TableDefinitionEditorView::editingFinished,
            this, &ProjectView::showCityLayout);

    setCurrentWidget(m_openOrCreateProjectView);

    connect(m_openOrCreateProjectView,
            &OpenOrCreateProjectView::projectTitleAccepted,
            this, &ProjectView::initializeNewProject);

    connect(m_openOrCreateProjectView,
            &OpenOrCreateProjectView::openLayoutRequested,
            this, &ProjectView::openLayoutRequested);

    connect(m_openOrCreateProjectView,
            &OpenOrCreateProjectView::openLastLayoutRequested,
            this, &ProjectView::openLastLayoutRequested);
}


ProjectView::~ProjectView()
{
    // LoadedProjects deletes element widgets before their parent view is destroyed.
    _Projects.clear();
    delete m_tableDefinitionEditor;
    delete m_cityLayoutView;
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
    if (auto *mainWindow = qobject_cast<QMainWindow *>(window())) {
        mainWindow->statusBar()->showMessage(
            tr("Table definition saved to %1").arg(QDir::toNativeSeparators(path)), 5000);
    }
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
    const bool metric = UserSettings::instance().measurementSystem() == UserSettings::MeasurementSystem::Metric;
    m_unitsAction->setToolTip(tr("Units: %1").arg(metric ? tr("Centimeters") : tr("Inches")));
}
