#include <QtMath>
#include "ProjectView.h"

#include "CityLayoutView.h"
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
    : QStackedWidget(parent)
    , m_openOrCreateProjectView(new OpenOrCreateProjectView(this))
    , m_cityLayoutView(new CityLayoutView(m_tableDefinition, this))
    , m_tableDefinitionEditor(
          new TableDefinitionEditorView(m_tableDefinition, this))
{
    addWidget(m_openOrCreateProjectView);
    addWidget(m_cityLayoutView);
    addWidget(m_tableDefinitionEditor);
    setCurrentWidget(m_openOrCreateProjectView);

    connect(m_tableDefinitionEditor, &TableDefinitionEditorView::editingFinished,
            this, &ProjectView::showCityLayout);

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

    // emit projectStarted(title);
}


void ProjectView::startProject(const QString &title)
{
    m_tableDefinition.clear();
    m_tableDefinitionEditor->reset();
    m_tableDefinitionEditor->resetViewScale();

    m_cityLayoutView->startProject(title);
    m_savedTableState = tableState();
    m_savedLayoutRevision = m_cityLayoutView->changeRevision();
}


QString ProjectView::projectTitle() const
{
    return m_cityLayoutView->projectTitle();
}


void ProjectView::defineTableOutline()
{
    setCurrentWidget(m_tableDefinitionEditor);
    m_tableDefinitionEditor->begin();
}


void ProjectView::showCityLayout()
{
    if (projectTitle().isEmpty()) {
        setCurrentWidget(m_openOrCreateProjectView);
        return;
    }

    setCurrentWidget(m_cityLayoutView);
    m_cityLayoutView->setFocus(Qt::OtherFocusReason);
    m_cityLayoutView->update();
}


bool ProjectView::saveTableDefinition()
{
    if (projectTitle().isEmpty()) {
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

    QString filename = projectTitle().trimmed();
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
    for (const TableSurface &surface : m_tableDefinition.surfaces()) {
        QJsonArray outline;
        for (const QPointF &point : surface.outline()) {
            outline.append(pointJson(point));
        }
        surfaces.append(QJsonObject{{QStringLiteral("name"), surface.name()},
                                    {QStringLiteral("outline"), outline}});
    }
    QJsonArray openSides;
    for (const QLineF &side : m_tableDefinition.openSides()) {
        openSides.append(QJsonObject{{QStringLiteral("start"), pointJson(side.p1())},
                                     {QStringLiteral("end"), pointJson(side.p2())}});
    }
    const QJsonDocument document(QJsonObject{
        {QStringLiteral("format"), QStringLiteral("LegoCityLayout.TableDefinition")},
        {QStringLiteral("version"), 1},
        {QStringLiteral("layoutName"), projectTitle()},
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
    m_savedTableState = tableState();
    m_savedLayoutRevision = m_cityLayoutView->changeRevision();
    settings.setValue(QStringLiteral("tableDefinition/saveDirectory"), directory);
    if (auto *mainWindow = qobject_cast<QMainWindow *>(window())) {
        mainWindow->statusBar()->showMessage(
            tr("Table definition saved to %1").arg(QDir::toNativeSeparators(path)), 5000);
    }
    return true;
}


void ProjectView::closeProject()
{
    if (!projectTitle().isEmpty()
        && (tableState() != m_savedTableState
            || m_cityLayoutView->changeRevision() != m_savedLayoutRevision)) {
        const auto choice = QMessageBox::warning(
            this, tr("Save Changes?"),
            tr("Save changes to \"%1\" before closing?").arg(projectTitle()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);
        if (choice == QMessageBox::Cancel
            || (choice == QMessageBox::Save && !saveTableDefinition())) {
            return;
        }
    }

    startProject(QString());
    setCurrentWidget(m_openOrCreateProjectView);
    m_openOrCreateProjectView->setFocus(Qt::OtherFocusReason);
    if (auto *mainWindow = qobject_cast<QMainWindow *>(window())) {
        mainWindow->setWindowTitle(tr("Lego City Layout"));
        mainWindow->statusBar()->clearMessage();
    }
}


QByteArray ProjectView::tableState() const
{
    const auto pointJson = [this](const QPointF &point) {
        const QPointF position = m_cityLayoutView->projectPoint(point);
        // Ignore floating point noise introduced by view zooming.
        return QJsonArray{qRound64(position.x() * 1000000.0),
                          qRound64(position.y() * 1000000.0)};
    };
    QJsonArray surfaces;
    for (const TableSurface &surface : m_tableDefinition.surfaces()) {
        QJsonArray outline;
        for (const QPointF &point : surface.outline()) {
            outline.append(pointJson(point));
        }
        surfaces.append(QJsonObject{{QStringLiteral("name"), surface.name()},
                                    {QStringLiteral("outline"), outline}});
    }
    QJsonArray sides;
    for (const QLineF &side : m_tableDefinition.openSides()) {
        sides.append(QJsonArray{pointJson(side.p1()), pointJson(side.p2())});
    }
    return QJsonDocument(QJsonObject{{QStringLiteral("surfaces"), surfaces},
                                    {QStringLiteral("openSides"), sides}})
        .toJson(QJsonDocument::Compact);
}
