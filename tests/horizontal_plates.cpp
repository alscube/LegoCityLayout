#include "CityLayoutView.h"
#include "LoadedProjects.h"
#include "elements/CityLayoutElement.h"
#include "elements/ElementCatalog.h"
#include "elements/LayoutElementsView.h"
#include "elements/LayoutElement.h"
#include <QApplication>
#include <QDebug>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QMimeData>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    LayoutElementsView palette;
    palette.addLayoutElements();
    auto &catalog = ElementCatalog::instance();
    auto &projects = LoadedProjects::instance();
    projects.setCurrentProjectIndex(projects.addProject("Horizontal plates"));
    CityLayoutView city;
    city.resize(1000, 1000);
    struct Cleanup { LoadedProjects &projects; ~Cleanup() { projects.clear(); } } cleanup{projects};
    projects.currentProject()->zoomFactor = 1.0;
    int checked = 0;
    for (auto *child : palette.findChildren<QWidget *>()) {
        auto *widget = dynamic_cast<LayoutElement *>(child);
        if (!widget) continue;
        if (widget->plateSize() != QSize(32, 16)) continue;
        const auto labels = widget->findChildren<QLabel *>();
        if (labels.size() != 2 || labels[0]->pixmap().width() != 2 * labels[0]->pixmap().height()) return 1;
        const auto *definition = catalog.byName(labels[1]->text());
        if (!definition) return 2;
        const QPixmap native(definition->resourcePath);
        if (native.width() != 1254 || native.height() != 627) return 9;
        for (bool rotated : {false, true}) {
            QMimeData mime;
            mime.setData("application/x-legocity-layout-element", definition->resourcePath.toUtf8());
            mime.setData("application/x-legocity-layout-element-name", definition->displayName.toUtf8());
            QJsonObject size{{"widthStuds", 32}, {"heightStuds", 16}};
            if (rotated) size.insert("rotationDegrees", 90.0);
            mime.setData("application/x-legocity-layout-element-size", QJsonDocument(size).toJson());
            QDragEnterEvent enter(QPoint(400, 400), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(&city, &enter);
            QDropEvent drop(QPointF(400, 400), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(&city, &drop);
            if (!drop.isAccepted()) return 3;
            auto *plate = projects.cityLayouts().last();
            if (plate->plateSize() != QSize(32, 16) || plate->rotationDegrees() != (rotated ? 90.0 : 0.0)) return 4;
            if (rotated ? qAbs(plate->height() - 2 * plate->width()) > 1 : qAbs(plate->width() - 2 * plate->height()) > 1) return 5;
            if (!rotated) {
                const auto saved = plate->savedPixmap();
                if (saved.width() != 2 * saved.height()) return 6;
                plate->rotateQuarterTurns(1);
                if (qAbs(plate->height() - 2 * plate->width()) > 1) return 7;
            }
        }
        ++checked;
    }
    if (checked != 3) return 8;
    projects.clear();
    qInfo() << "PASS: all three 16x32 plate previews, native landscape files, horizontal drops, rotated drops, saved artwork and subsequent rotation";
    return 0;
}
