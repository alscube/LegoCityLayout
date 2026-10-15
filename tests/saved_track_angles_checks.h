#pragma once

static int checkSavedTrackAngles(LoadedProjects &projects)
{
    const int original = projects.currentProjectIndex(), count = projects.projectCount();
    struct Cleanup {
        LoadedProjects &projects; int count, original;
        ~Cleanup() {
            while (projects.projectCount() > count) projects.removeProject(projects.projectCount()-1);
            projects.setCurrentProjectIndex(original);
        }
    } cleanup{projects, count, original};
    const QString path = QFileInfo(QString::fromUtf8(__FILE__)).dir()
        .filePath("fixtures/curved_track_angles.LegoLayout");
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return 110;
    const auto city = QJsonDocument::fromJson(file.readAll()).object().value("cityLayout").toObject();
    const auto placements = city.value("elements").toArray();
    ProjectView editor;
    // Close an error dialog on failure so a loader regression cannot hang the test.
    QTimer dismissError;
    QObject::connect(&dismissError, &QTimer::timeout, [] {
        for (auto *widget : QApplication::topLevelWidgets())
            if (auto *box = qobject_cast<QMessageBox *>(widget)) box->accept();
    });
    dismissError.start(20);
    if (!editor.openLayout(path)) return 111;
    CityLayoutView view;
    const auto saved = view.savedLayout();
    const auto *plateDefinition=ElementCatalog::instance().byName(placements[0].toObject().value("name").toString());
    if (!plateDefinition || saved != city || placements.size() != 25
        || projects.cityLayouts().first()->name() != plateDefinition->name
        || projects.cityLayouts().first()->resourcePath() != plateDefinition->resourcePath) return 112;
    // Reopen the serialized placements and verify every angle and position again.
    QTemporaryDir temporary;
    if (!temporary.isValid()) return 113;
    file.seek(0);
    auto root = QJsonDocument::fromJson(file.readAll()).object();
    root.insert("cityLayout", saved);
    QFile roundTrip(temporary.filePath("roundtrip.LegoLayout"));
    if (!roundTrip.open(QIODevice::WriteOnly)) return 114;
    roundTrip.write(QJsonDocument(root).toJson());
    roundTrip.close();
    if (!editor.openLayout(roundTrip.fileName()) || view.savedLayout() != city) return 115;
    const int loadedCount = projects.projectCount();
    const auto rejects = [&](int index, qreal angle) {
        auto invalidElements = placements;
        auto element = invalidElements[index].toObject();
        element.insert("rotationDegrees", angle);
        invalidElements[index] = element;
        auto invalidCity = city;
        invalidCity.insert("elements", invalidElements);
        auto invalidRoot = root;
        invalidRoot.insert("cityLayout", invalidCity);
        QFile invalid(temporary.filePath("invalid.LegoLayout"));
        if (!invalid.open(QIODevice::WriteOnly)) return false;
        invalid.write(QJsonDocument(invalidRoot).toJson());
        invalid.close();
        return !editor.openLayout(invalid.fileName()) && projects.projectCount() == loadedCount;
    };
    if (!rejects(0, 22.5) || !rejects(2, -1.0) || !rejects(2, 360.0)) return 116;
    qInfo() << "PASS: supplied 25-element layout preserves aligned track angles and positions across reloads; invalid rotations rejected";
    return 0;
}
