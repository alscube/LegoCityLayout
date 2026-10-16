#include "CityLayoutView.h"
#include "LoadedProjects.h"
#include "LegoGrid.h"
#include "elements/CityLayoutElement.h"
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QJsonArray>
#include <QKeySequence>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    auto &projects = LoadedProjects::instance();
    projects.setCurrentProjectIndex(projects.addProject("Undo deletion"));
    CityLayoutView city;
    struct Cleanup { LoadedProjects &projects; ~Cleanup() { projects.clear(); } } cleanup{projects};
    auto *project = projects.currentProject();
    QPixmap artwork(16, 32);
    artwork.fill(Qt::green);
    auto add = [&](const QString &name) {
        auto *element = new CityLayoutElement(name, artwork, QSize(16,32),
            LegoGrid::pixelsPerStud, project->zoomFactor, &city);
        project->cityLayouts.append(element);
        return element;
    };
    auto *bottom = add("Bottom");
    auto *middle = add("Middle");
    auto *top = add("Top");
    middle->rotateByDegrees(90);
    middle->setLayoutPosition(QPointF(112.125,73.375));
    const QPointF logical = (middle->layoutPosition() - project->gridOrigin) / project->zoomFactor;
    const QPixmap source = middle->sourcePixmap();
    project->cityLayouts.setSelectedElement(middle);
    auto key = [&](int code, Qt::KeyboardModifiers mods = Qt::NoModifier) {
        QKeyEvent event(QEvent::KeyPress, code, mods);
        QApplication::sendEvent(&city, &event);
    };
    const auto undo = QKeySequence(QKeySequence::Undo)[0];
    key(Qt::Key_Delete);
    if (project->cityLayouts.size() != 2 || !project->cityLayouts.canUndoDeletion()
        || project->cityLayouts.selectedElement() || project->changeRevision != 1) return 1;
    if (city.savedLayout().value("elements").toArray().size() != 2) return 2;
    projects.setViewZoom(QPointF(33.25,47.75), 3.0);
    // A pan changes the view origin, while deleted elements retain project coordinates.
    project->gridOrigin += QPointF(50, 25);
    key(undo.key(), undo.keyboardModifiers());
    middle = project->cityLayouts.selectedElement();
    if (project->cityLayouts.size() != 3 || project->cityLayouts[0] != bottom
        || project->cityLayouts[1] != middle || project->cityLayouts[2] != top
        || middle->name() != "Middle" || middle->rotationDegrees() != 90
        || middle->sourcePixmap().cacheKey() != source.cacheKey()
        || QLineF(middle->layoutPosition(), project->gridOrigin + logical * project->zoomFactor).length() > 1e-8
        || project->changeRevision != 2) return 3;
    // Consecutive deletions are restored in reverse order and stay project-local.
    key(Qt::Key_Backspace);
    project->cityLayouts.setSelectedElement(top);
    key(Qt::Key_Delete);
    const int other = projects.addProject("Other project");
    projects.setCurrentProjectIndex(other);
    key(undo.key(), undo.keyboardModifiers());
    if (!projects.cityLayouts().isEmpty() || projects.cityLayouts().canUndoDeletion()) return 4;
    projects.setCurrentProjectIndex(0);
    key(undo.key(), undo.keyboardModifiers());
    if (project->cityLayouts.last()->name() != "Top") return 5;
    key(undo.key(), undo.keyboardModifiers());
    if (project->cityLayouts[1]->name() != "Middle" || project->cityLayouts.canUndoDeletion()) return 6;
    const quint64 revision = project->changeRevision;
    key(undo.key(), undo.keyboardModifiers());
    if (project->changeRevision != revision) return 7;
    // Clearing a layout clears deletion history.
    key(Qt::Key_Delete);
    project->cityLayouts.initializeCityElements(QString());
    if (project->cityLayouts.canUndoDeletion()) return 8;
    auto *flex = new CityLayoutElement("Flex", artwork, QSize(8,10),
        LegoGrid::pixelsPerStud, project->zoomFactor, &city, ":/images/FlexTrack.png");
    project->cityLayouts.append(flex);
    flex->setFlexBendDegrees(3.0);
    flex->rotateByDegrees(22.5);
    project->cityLayouts.setSelectedElement(flex);
    key(Qt::Key_Delete);
    key(undo.key(), undo.keyboardModifiers());
    flex = project->cityLayouts.selectedElement();
    if (!flex->isFlexTrack() || flex->rotationDegrees() != 22.5
        || qAbs(flex->flexBendDegrees() - 3.0) > 1e-8) return 9;
    qInfo() << "PASS: delete/backspace and native undo restore artwork, rotation, position after zoom/pan, stacking and selection; repeated deletion, project isolation and history cleanup";
    return 0;
}
