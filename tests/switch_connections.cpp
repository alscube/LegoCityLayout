#include "panning_checks.h"
#include "elements/CityLayoutElement.h"
#include "CityLayoutView.h"
#include "elements/LayoutElementsView.h"
#include "elements/StraightTrack.h"
#include "elements/LeftSwitchTrack.h"
#include "elements/RightSwitchTrack.h"
#include <QLabel>
#include <type_traits>
#include "LoadedProjects.h"
#include "ProjectView.h"
#include "mainwindow.h"
#include "UserSettings.h"
#include "LegoGrid.h"
#include <QApplication>
#include <QMimeData>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QSettings>
#include <QTabWidget>
#include <QDebug>
#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QAbstractButton>
#include <cmath>
#include <array>
#include <memory>

static bool near(qreal a, qreal b) { return qAbs(a - b) < 1e-5; }

#include "flex_checks.h"
#include "track_run_checks.h"
#include "zoom_checks.h"
#include "compact_save_checks.h"
#include "saved_track_angles_checks.h"
#include "curve_grid_checks.h"
#include "plate_info_checks.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (const int failure=checkPlateInfo()) return failure;
    if (!checkPanningRendering()) return 45;
    // Keep the application's remembered layout path out of this check.
    QTemporaryDir temporary;
    if (!temporary.isValid()) return 1;
    QCoreApplication::setOrganizationName("LegoCityLayoutTests");
    QCoreApplication::setApplicationName("SwitchConnections");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
    auto &projects = LoadedProjects::instance();
    if (const int failure=checkCurveGrid(projects)) return failure;
    if (const int failure=checkCompactSave(projects)) return failure;
    if (const int failure=checkSavedTrackAngles(projects)) return failure;
    const int index = projects.addProject("Switch checks");
    projects.setCurrentProjectIndex(index);
    if (const int failure=checkZoom(projects)) return failure;
    if (const int failure=checkTrackRun(projects)) return failure;
    CityLayoutView view;
    // The topmost element owns its entire tight selection rectangle, including
    // transparent mounting holes and gaps between sleepers/rails.
    QPixmap plateImage(16,16);
    plateImage.fill(Qt::green);
    CityLayoutElement underlay("Underlay",plateImage,QSize(64,64),8.0);
    const std::array<QString,5> trackPaths{":/images/StrightTrack.png",":/images/CurvedTrack.png",
        ":/images/LeftSwitchTrack.png",":/images/RightSwitchTrack.png",":/images/FlexTrack.png"};
    // Artwork and dimensions alone must not assign a track identity.
    for (int type=0;type<2;++type) {
        const QSize size=type==0 ? QSize(16,8) : QSize(17,11);
        CityLayoutElement anonymous("Anonymous",QPixmap(trackPaths[type]),size,LegoGrid::pixelsPerStud);
        if (anonymous.isTrack() || !anonymous.resourcePath().isEmpty()
            || !anonymous.trackConnections().isEmpty()) return 24;
    }
    for (int type=0;type<5;++type) {
        const QSize size=type==0 ? QSize(16,8) : type==1 ? QSize(17,11) : type==4 ? QSize(8,10) : QSize(36,24);
        CityLayoutElement top("Top",QPixmap(trackPaths[type]),size,LegoGrid::pixelsPerStud,1.0,nullptr,trackPaths[type]);
        CityLayoutElements layers;
        layers.append(&underlay);
        layers.append(&top);
        for (qreal zoom : {1.875,4.0}) {
            top.setZoomFactor(zoom);
            for (qreal turn : {0.0,22.5,90.0,157.5}) {
                top.rotateByDegrees(turn-top.rotationDegrees());
                top.move(50,50);
                const QImage artwork=top.pixmap().toImage().scaled(top.size());
                QPoint hole(-1,-1);
                for (int y=3;y<artwork.height()-3 && hole.x()<0;++y) for (int x=3;x<artwork.width()-3;++x) {
                    if (!top.containsPoint(QPoint(x,y))) continue;
                    bool transparent=true;
                    for (int dy=-2;dy<=2 && transparent;++dy) for (int dx=-2;dx<=2;++dx) {
                        if (qAlpha(artwork.pixel(x+dx,y+dy))>=32) {transparent=false;break;}
                    }
                    if (transparent) {hole=QPoint(x,y);break;}
                }
                if (hole.x()<0) return 14;
                const QPoint click=top.pos()+hole;
                if (layers.elementAt(click)!=&top) return 15;
                layers.setSelectedElementAt(click);
                if (layers.selectedElement()!=&top || !layers.wasElementClicked(&view,click)
                    || layers.draggedElement()!=&top) return 16;
                if (layers.elementAt(QPoint(0,0))!=&underlay) return 17;
                layers.setDraggedElement(nullptr);
            }
        }
    }
    // A straight drops at zero degrees with a 16x8 footprint and left/right ports.
    {
        const QString resource = ":/images/StrightTrack.png";
        QMimeData mime;
        mime.setData("application/x-legocity-layout-element", resource.toUtf8());
        mime.setData("application/x-legocity-layout-element-name", "Straight");
        mime.setData("application/x-legocity-layout-element-size", QJsonDocument(QJsonObject{
            {"widthStuds", 16}, {"heightStuds", 8}}).toJson(QJsonDocument::Compact));
        QDragEnterEvent enter(QPoint(500,500), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&view, &enter);
        QDropEvent drop(QPointF(500,500), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&view, &drop);
        if (!drop.isAccepted() || projects.cityLayouts().size()!=1) return 48;
        auto *placed = projects.cityLayouts().last();
        const auto ports = placed->trackConnections();
        const QRectF body = placed->footprintRect();
        if (placed->plateSize()!=QSize(16,8) || !near(placed->rotationDegrees(),0)
            || placed->sourcePixmap().toImage()!=QPixmap(resource).toImage()
            || placed->width()<=placed->height() || ports.size()!=2
            || !near(ports[0].p1().x(),body.left()) || !near(ports[1].p1().x(),body.right())
            || !near(ports[0].p1().y(),body.center().y()) || !near(ports[1].p1().y(),body.center().y())
            || ports[0].p2().x()>=ports[0].p1().x() || ports[1].p2().x()<=ports[1].p1().x()) return 49;
        placed->setZoomFactor(12.0); // Resolve the hole interiors before checking alpha.
        const QImage artwork = placed->pixmap().toImage();
        // The three mounting holes follow the horizontal sleeper centers.
        for (qreal fraction : {364.0/1344,674.0/1344,984.0/1344}) {
            if (qAlpha(artwork.pixel(qRound(fraction * artwork.width()), artwork.height()/2))!=0) return 50;
        }
        projects.cityLayouts().deleteImage(placed);
    }
    // Dropping either switch must preserve the horizontal palette orientation.
    for (const QString &resource : {QString(":/images/LeftSwitchTrack.png"), QString(":/images/RightSwitchTrack.png")}) {
        QMimeData mime;
        mime.setData("application/x-legocity-layout-element", resource.toUtf8());
        mime.setData("application/x-legocity-layout-element-name", "Switch");
        mime.setData("application/x-legocity-layout-element-size", QJsonDocument(QJsonObject{
            {"widthStuds", 36}, {"heightStuds", 24}}).toJson(QJsonDocument::Compact));
        QDragEnterEvent enter(QPoint(500,500), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&view, &enter);
        QDropEvent drop(QPointF(500,500), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(&view, &drop);
        if (!drop.isAccepted() || projects.cityLayouts().size()!=1) return 46;
        auto *placed = projects.cityLayouts().last();
        const auto connections = placed->trackConnections();
        const QPixmap rendered = placed->pixmap();
        QImage expected(rendered.size(), QImage::Format_ARGB32_Premultiplied);
        expected.fill(Qt::transparent);
        QPainter reference(&expected);
        reference.setRenderHint(QPainter::SmoothPixmapTransform);
        const qreal pitch = LegoGrid::pixelsPerStud * projects.currentProject()->zoomFactor * rendered.devicePixelRatioF();
        reference.drawPixmap(QRectF(0,0,36*pitch,24*pitch),QPixmap(resource),QRectF(QPixmap(resource).rect()));
        reference.end();
        if (placed->width() <= placed->height() || connections.size()!=3
            || placed->plateSize()!=QSize(36,24) || !near(placed->rotationDegrees(),0)
            || placed->sourcePixmap().toImage()!=QPixmap(resource).toImage()
            || !near(connections[0].p1().y(), connections[1].p1().y())
            || rendered.toImage()!=expected) return 47;
        projects.cityLayouts().deleteImage(placed);
    }
    const QSize switchSize(36, 24);
    int checks = 0;
    // A switch plus a return curve ends 16 studs to the side and 48 studs
    // from its entrance, level with a straight attached to the through route.
    const qreal sweep = qDegreesToRadians(LegoGrid::curveSweepDegrees);
    if (qAbs(LegoGrid::switchBranchOffsetStuds + LegoGrid::curveRadiusStuds*(1-std::cos(sweep)) - 16.0)>0.0001
        || qAbs(LegoGrid::switchBranchLengthStuds + LegoGrid::curveRadiusStuds*std::sin(sweep) - 48.0)>0.0001) return 13;
    for (const QString &resource : {QString(":/images/LeftSwitchTrack.png"), QString(":/images/RightSwitchTrack.png")}) {
        const QPixmap image(resource);
        if (image.isNull()) return 2;
        auto *sw = new CityLayoutElement(ElementCatalog::instance().byResource(resource)->name, image, switchSize, LegoGrid::pixelsPerStud, 1.0, &view, resource);
        projects.cityLayouts().append(sw);
        sw->move(200, 200);
        if (!sw->isSwitchTrack() || sw->trackConnections().size() != 3 || sw->rotationStepDegrees() != 22.5) return 3;
        const auto ports = sw->trackConnections();
        const qreal pitchX = LegoGrid::pixelsPerStud;
        const qreal pitchY = LegoGrid::pixelsPerStud;
        const qreal sign = resource.contains("Right") ? 1 : -1;
        const qreal entranceX = sign > 0 ? 1.0 : 35.0;
        if (!near(ports[0].p1().x()/pitchX, entranceX)
            || !near(ports[0].p1().y()/pitchY, 20.0)
            || !near((ports[1].p1().x()-ports[0].p1().x())/pitchX, sign*32.0)
            || !near(ports[1].p1().y(), ports[0].p1().y())
            || !near((ports[2].p1().x()-ports[0].p1().x())/pitchX, sign*LegoGrid::switchBranchLengthStuds)
            || !near((ports[2].p1().y()-ports[0].p1().y())/pitchY, -LegoGrid::switchBranchOffsetStuds)
            || (ports[0].p2().x()-ports[0].p1().x())*sign >= 0
            || (ports[1].p2().x()-ports[1].p1().x())*sign <= 0) return 4;
        // Every end, both hands, every 22.5-degree rotation, multiple zoom levels.
        for (qreal zoom : {1.875, 4.0, 8.0}) {
            projects.currentProject()->zoomFactor = zoom;
            sw->setZoomFactor(zoom);
            std::array<std::unique_ptr<CityLayoutElement>, 32> movingTracks;
            for (int type=0; type<2; ++type) for (int turn=0; turn<16; ++turn) {
                const QString path=type ? ":/images/CurvedTrack.png" : ":/images/StrightTrack.png";
                auto track=std::make_unique<CityLayoutElement>("Track",QPixmap(path),type ? QSize(17,11) : QSize(16,8),
                    LegoGrid::pixelsPerStud,zoom,nullptr,path);
                track->rotateByDegrees(turn*22.5);
                movingTracks[type*16+turn]=std::move(track);
            }
            for (int step=0; step<16; ++step) {
                for (const auto &target : sw->trackConnections()) {
                    for (bool curved : {false, true}) {
                        const QPointF direction = target.p2()-target.p1();
                        const qreal targetAngle=qRadiansToDegrees(std::atan2(direction.y(),direction.x()));
                        const int turn=(qRound(targetAngle/22.5)%16+16)%16;
                        CityLayoutElement &moving=*movingTracks[(curved ? 16 : 0)+turn];
                        const QLineF movingPort=moving.trackConnections().first();
                        const QPointF ideal=sw->layoutPosition()+target.p1()-movingPort.p1();
                        const QPoint requested=ideal.toPoint()+QPoint(3,-2);
                        const QPointF snapped=view.snappedPosition(requested,&moving);
                        if (QLineF(QPointF(snapped)+movingPort.p1(),sw->layoutPosition()+target.p1()).length() > 0.75) {
                            qCritical() << "Snap failed" << resource << zoom << step << curved << target << snapped << ideal;
                            return 5;
                        }
                        // Exercise dragging the switch onto an existing track as well.
                        projects.cityLayouts().removeAll(sw);
                        moving.setLayoutPosition(snapped);
                        projects.cityLayouts().append(&moving);
                        const QPointF reversed=view.snappedPosition(sw->pos()+QPoint(3,-2),sw);
                        if (QLineF(QPointF(reversed)+target.p1(),moving.layoutPosition()+moving.trackConnections().first().p1()).length()>0.75) return 6;
                        projects.cityLayouts().removeAll(&moving);
                        projects.cityLayouts().append(sw);
                        ++checks;
                    }
                }
                sw->rotateSteps(1);
            }
        }
        sw->rotateByDegrees(22.5);
    }
    if (const int failure=checkFlexTrack(view,projects)) return failure;
    const QString straightResource=":/images/StrightTrack.png";
    const QPixmap horizontalStraight(straightResource);
    if (horizontalStraight.size()!=QSize(1377,1142)) return 42;
    auto *verticalStraight=new CityLayoutElement("Straight Track",horizontalStraight,
        QSize(16,8),LegoGrid::pixelsPerStud,projects.currentProject()->zoomFactor,&view,straightResource);
    auto *newStraight=new CityLayoutElement("Straight Track",horizontalStraight,
        QSize(16,8),LegoGrid::pixelsPerStud,projects.currentProject()->zoomFactor,&view,straightResource);
    verticalStraight->rotateByDegrees(90);
    verticalStraight->move(2000,1000);
    newStraight->move(2300,1000);
    projects.cityLayouts().append(verticalStraight);
    projects.cityLayouts().append(newStraight);
    const auto verticalPorts=verticalStraight->trackConnections();
    const auto horizontalPorts=newStraight->trackConnections();
    // The third connector and handedness survive the real JSON save/load path.
    const std::array<QList<QLineF>,2> switchPorts{projects.cityLayouts().at(0)->trackConnections(),
                                               projects.cityLayouts().at(1)->trackConnections()};
    const QJsonObject saved=view.savedLayout();
    const auto savedElements=saved.value("elements").toArray();
    const QString path=temporary.filePath("switches.LegoLayout");
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return 7;
    file.write(QJsonDocument(QJsonObject{{"format","LegoCityLayout.TableDefinition"},{"version",2},
        {"layoutName","Restored switches"},{"surfaces",QJsonArray()},{"openSides",QJsonArray()},{"cityLayout",saved}}).toJson());
    file.close();
    ProjectView projectView;
    if (!projectView.openLayout(path) || projects.cityLayouts().size()!=5) return 8;
    if (projects.currentProject()->filePath!=QFileInfo(path).absoluteFilePath()) return 110;
    for (int i=0;i<2;++i) {
        const auto *sw=projects.cityLayouts().at(i);
        if (!sw->isSwitchTrack() || sw->trackConnections().size()!=3
            || sw->resourcePath()!=QString(i==0 ? ":/images/LeftSwitchTrack.png" : ":/images/RightSwitchTrack.png")
            || !near(sw->rotationDegrees(),savedElements[i].toObject().value("rotationDegrees").toDouble())) return 9;
        if (sw->containsPoint(QPoint(-1,-1))) return 10;
        if (sw->plateSize()!=QSize(36,24)) return 51;
        const auto restoredPorts = sw->trackConnections();
        for (int port=0;port<3;++port) {
            if (QLineF(restoredPorts[port].p1(),switchPorts[i][port].p1()).length()>1e-6
                || QLineF(restoredPorts[port].p2(),switchPorts[i][port].p2()).length()>1e-6) return 52;
        }
    }
    const auto *restoredFlex=projects.cityLayouts().at(2);
    if (!restoredFlex->isFlexTrack() || restoredFlex->trackConnections().size()!=2
        || !near(restoredFlex->flexBendDegrees(),1.25) || !near(restoredFlex->rotationDegrees(),11.25)) return 35;
    const auto *reopenedVertical=projects.cityLayouts().at(3);
    const auto *reopenedHorizontal=projects.cityLayouts().at(4);
    if (!near(reopenedVertical->rotationDegrees(),90) || reopenedVertical->height()<=reopenedVertical->width()
        || !near(reopenedHorizontal->rotationDegrees(),0) || reopenedHorizontal->width()<=reopenedHorizontal->height()) return 43;
    for (int end=0;end<2;++end) {
        if (QLineF(verticalPorts[end].p1(),reopenedVertical->trackConnections()[end].p1()).length()>1e-6
            || QLineF(horizontalPorts[end].p1(),reopenedHorizontal->trackConnections()[end].p1()).length()>1e-6) return 44;
    }
    // An 8x16 plate remains a plate, and both switches appear in the palette.
    CityLayoutElement plate("Plate", QPixmap(":/images/BasePlate16x32.png"), QSize(8,16), LegoGrid::pixelsPerStud,
                            1.0,nullptr,":/images/BasePlate8x16Green.png");
    if (plate.isTrack() || !plate.trackConnections().isEmpty() || plate.rotationStepDegrees()!=90) return 11;
    LayoutElementsView palette;
    palette.addLayoutElements();
    auto *tabs=palette.findChild<QTabWidget *>("layoutElementsTabs");
    QWidget *trackPage=nullptr;
    if (tabs) for (int i=0;i<tabs->count();++i)
        if (tabs->tabText(i)=="Tracks") trackPage=tabs->widget(i);
    if (!trackPage || trackPage->findChildren<QWidget *>("layoutElementWidget").size()!=5) return 12;
    static_assert(std::is_base_of<LayoutElement,StraightTrack>::value);
    StraightTrack *straightPalette=nullptr;
    int switchPaletteCount=0;
    for (auto *widget : trackPage->findChildren<QWidget *>("layoutElementWidget")) {
        if (auto *straight=dynamic_cast<StraightTrack *>(widget)) straightPalette=straight;
        if (dynamic_cast<LeftSwitchTrack *>(widget) || dynamic_cast<RightSwitchTrack *>(widget)) {
            const auto *element=dynamic_cast<LayoutElement *>(widget);
            if (element->plateSize()!=QSize(36,24)) return 53;
            ++switchPaletteCount;
        }
    }
    if (switchPaletteCount!=2) return 54;
    if (!straightPalette || straightPalette->plateSize()!=QSize(16,8)) return 40;
    class ReferenceElement final : public LayoutElement {
    public:
        ReferenceElement() : LayoutElement("Reference",":/images/StrightTrack.png",QSize(16,8)) {
            initElementImage();
        }
    };
    ReferenceElement fullSize;
    const auto imageSize=[](const LayoutElement &element) {
        for (auto *label : element.findChildren<QLabel *>()) {
            if (!label->pixmap().isNull()) return label->pixmap().size();
        }
        return QSize();
    };
    if (imageSize(*straightPalette)!=imageSize(fullSize)*0.50) return 41;
    palette.resize(210,800);
    tabs->setCurrentWidget(trackPage);
    palette.show();
    app.processEvents();
    palette.grab().save("switch-palette-preview.png");
    projects.clear();
    // Exercise the actual window-close event and modal Save/Discard/Cancel UI.
    app.setQuitOnLastWindowClosed(false);
    {
        MainWindow window;
        const auto requestClose=[&] {
            window.show();
            app.processEvents();
            return window.close();
        };
        auto *editor=window.findChild<ProjectView *>();
        if (!editor) return 18;
        UserSettings::instance().setLayoutSaveDirectory(QString());
        editor->startProject("Close checks");
        if (!requestClose()) return 19; // Unchanged layout closes without a prompt.
        ++projects.currentProject()->changeRevision;
        QTimer responder;
        responder.setInterval(5);
        int prompts=0;
        int dialogs=0;
        auto choice=QMessageBox::Cancel;
        bool saveFile=false;
        bool automaticOverwrite=false;
        QObject::connect(&responder,&QTimer::timeout,[&] {
            if (auto *box=qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
                ++prompts;
                box->button(choice)->click();
            } else if (auto *dialog=qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                ++dialogs;
                automaticOverwrite=dialog->testOption(QFileDialog::DontConfirmOverwrite);
                if (saveFile) {
                    dialog->selectFile(temporary.filePath("Close checks.LegoLayout"));
                    QMetaObject::invokeMethod(dialog,"accept",Qt::DirectConnection);
                } else dialog->reject();
            }
        });
        responder.start();
        if (requestClose() || prompts!=1 || projects.projectCount()!=1) return 20;
        choice=QMessageBox::Save;
        if (requestClose() || dialogs!=1 || projects.projectCount()!=1 || !automaticOverwrite) return 21;
        saveFile=true;
        if (!requestClose() || !QFile::exists(temporary.filePath("Close checks.LegoLayout"))) return 22;
        if (UserSettings::instance().layoutSaveDirectory()!=temporary.path()) return 114;
        const int previousDialogs=dialogs;
        projects.currentProject()->setTitle("Saved again on close");
        ++projects.currentProject()->changeRevision;
        if (!requestClose() || dialogs!=previousDialogs) return 111;
        QFile overwritten(temporary.filePath("Saved again on close.LegoLayout"));
        if (!overwritten.open(QIODevice::ReadOnly)
            || QJsonDocument::fromJson(overwritten.readAll()).object().value("layoutName")!="Saved again on close") return 112;
        if (projects.currentProject()->filePath!=QFileInfo(overwritten.fileName()).absoluteFilePath()) return 113;
        ++projects.currentProject()->changeRevision;
        if (!requestClose() || dialogs!=previousDialogs) return 115;
        editor->startProject("Auto named new layout");
        ++projects.currentProject()->changeRevision;
        if (!editor->saveTableDefinition() || dialogs!=previousDialogs
            || !QFile::exists(temporary.filePath("Auto named new layout.LegoLayout"))) return 116;
        projects.removeProject(projects.currentProjectIndex());
        editor->setCurrentProjectIndex(0);
        const int previousPrompts=prompts;
        if (!requestClose() || prompts!=previousPrompts) return 23; // Saved layouts stay clean.
        ++projects.currentProject()->changeRevision;
        choice=QMessageBox::Discard;
        if (!requestClose() || projects.currentProject()->changeRevision==projects.currentProject()->savedLayoutRevision) return 24;
        editor->startProject("Other dirty layout");
        ++projects.currentProject()->changeRevision;
        const int original=projects.currentProjectIndex();
        choice=QMessageBox::Cancel;
        const bool multipleClosed=requestClose();
        if (multipleClosed || projects.currentProjectIndex()!=original || projects.projectCount()!=2) {
            qCritical()<<"Multiple close"<<multipleClosed<<projects.currentProjectIndex()<<original<<projects.projectCount();
            return 25;
        }
        // Check loaded but inactive projects after closing the active layout.
        editor->setCurrentProjectIndex(-1);
        if (editor->confirmCloseAllProjects() || projects.currentProjectIndex()!=-1) return 26;
        responder.stop();
    }
    qInfo() << "PASS:" << checks << "switch/track combinations snapped both ways; save/reopen, selection, and Tracks palette";
    return 0;
}
