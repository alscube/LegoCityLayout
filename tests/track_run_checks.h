#pragma once

#include <QKeyEvent>
#include <QPointer>

static int checkTrackRun(LoadedProjects &projects)
{
    const int original = projects.currentProjectIndex();
    const int testProject = projects.addProject("Track run shortcuts");
    projects.setCurrentProjectIndex(testProject);
    CityLayoutView view;
    struct Cleanup {
        LoadedProjects &projects;
        int index;
        int original;
        ~Cleanup() { projects.removeProject(index); projects.setCurrentProjectIndex(original); }
    } cleanup{projects, testProject, original};
    auto &layout = projects.cityLayouts();
    const auto key = [&](int value, Qt::KeyboardModifiers modifiers = Qt::NoModifier) {
        QKeyEvent event(QEvent::KeyPress, value, modifiers);
        QApplication::sendEvent(&view, &event);
    };
    const auto drop = [&](const QString &path, const QSize &size, const QPoint &center) -> CityLayoutElement * {
        const auto count = layout.size();
        QMimeData mime;
        mime.setData("application/x-legocity-layout-element",path.toUtf8());
        mime.setData("application/x-legocity-layout-element-name","Track run");
        mime.setData("application/x-legocity-layout-element-size",QJsonDocument(QJsonObject{
            {"widthStuds",size.width()},{"heightStuds",size.height()}}).toJson(QJsonDocument::Compact));
        QDragEnterEvent enter(center,Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&view,&enter);
        QDropEvent event(QPointF(center),Qt::CopyAction,&mime,Qt::LeftButton,Qt::NoModifier);
        QApplication::sendEvent(&view,&event);
        return event.isAccepted() && layout.size()==count+1 ? layout.last() : nullptr;
    };
    const auto joined = [](const CityLayoutElement *first, int exit, const CityLayoutElement *second, int entry) {
        const QLineF a = first->trackConnections()[exit];
        const QLineF b = second->trackConnections()[entry];
        const QPointF da=(a.p2()-a.p1())/a.length(), db=(b.p2()-b.p1())/b.length();
        return QLineF(first->layoutPosition()+a.p1(),second->layoutPosition()+b.p1()).length()<=0.8
            && QPointF::dotProduct(da,db)<-0.996;
    };
    const std::array<QString,3> paths{":/images/StrightTrack.png",":/images/CurvedTrack.png",":/images/FlexTrack.png"};
    const std::array<QSize,3> sizes{QSize(16,8),QSize(17,11),QSize(8,10)};
    for (int type=0;type<3;++type) {
        auto *seed=drop(paths[type],sizes[type],QPoint(500,500));
        if (!seed) return 60;
        seed->rotateByDegrees(22.5);
        if (seed->isFlexTrack()) seed->setFlexBendDegrees(1.25);
        const auto before=view.changeRevision();
        key(Qt::Key_Plus,Qt::ShiftModifier);
        if (layout.size()!=2 || view.changeRevision()!=before+1) return 61;
        QPointer<CityLayoutElement> added=layout.selectedElement();
        if (!added || added==seed || added->resourcePath()!=paths[type]
            || !near(added->flexBendDegrees(),seed->flexBendDegrees()) || !joined(seed,1,added,0)) return 62;
        key(Qt::Key_Plus,Qt::ControlModifier);
        if (layout.size()!=2) return 63;
        // Drawing order and selection do not replace addition order.
        layout.setSelectedElement(seed);
        layout.bringSelectedToFront();
        key(Qt::Key_Plus,Qt::KeypadModifier);
        if (layout.size()!=3 || !joined(added,1,layout.selectedElement(),0)) return 64;
        QPointer<CityLayoutElement> tail=layout.selectedElement();
        key(Qt::Key_Minus);
        if (tail || layout.size()!=2 || layout.selectedElement()!=added) return 65;
        key(Qt::Key_Minus,Qt::KeypadModifier);
        if (added || layout.size()!=1 || layout.selectedElement()!=seed) return 66;
        key(Qt::Key_Equal);
        if (layout.size()!=2 || !joined(seed,1,layout.selectedElement(),0)) return 67;
        // Delete can remove the tail outside the run helper without leaving a dangling pointer.
        key(Qt::Key_Delete);
        key(Qt::Key_Plus);
        if (layout.size()!=2 || !joined(seed,1,layout.selectedElement(),0)) return 68;
        key(Qt::Key_Minus);
        QPointer<CityLayoutElement> root=seed;
        key(Qt::Key_Minus);
        if (root || !layout.isEmpty()) return 69;
        key(Qt::Key_Plus);
        key(Qt::Key_Minus);
        if (!layout.isEmpty()) return 70;
    }
    // A manually attached curve changes the repeated type and preserves the preceding straight.
    auto *seed=drop(paths[0],sizes[0],QPoint(500,500));
    if (!seed) return 71;
    const auto attach = [&](const QString &path,const QSize &size,int entrance) {
        CityLayoutElement prototype("Prototype",QPixmap(path),size,LegoGrid::pixelsPerStud,
                                    projects.currentProject()->zoomFactor,nullptr,path);
        const QPointF position=QPointF(seed->pos())+seed->trackConnections()[1].p1()
            -prototype.trackConnections()[entrance].p1();
        return drop(path,size,position.toPoint()+QPoint(prototype.width()/2,prototype.height()/2));
    };
    auto *curve=attach(paths[1],sizes[1],0);
    if (!curve || !joined(seed,1,curve,0)) return 72;
    key(Qt::Key_Plus);
    if (layout.size()!=3 || layout.selectedElement()->resourcePath()!=paths[1]
        || !joined(curve,1,layout.selectedElement(),0)) return 73;
    key(Qt::Key_Minus);
    key(Qt::Key_Minus);
    if (layout.size()!=1 || layout.selectedElement()!=seed) return 74;
    // A run joining the second connector continues from the first connector.
    seed->rotateByDegrees(180);
    auto *reverse=attach(paths[0],sizes[0],1);
    if (!reverse || !joined(seed,1,reverse,1)) return 75;
    key(Qt::Key_Plus);
    if (layout.size()!=3 || !joined(reverse,0,layout.selectedElement(),1)) return 76;
    key(Qt::Key_Minus);
    key(Qt::Key_Minus);
    // Tracking is project-local, so shortcuts in another project do not modify this run.
    const int other=projects.addProject("Other run");
    projects.setCurrentProjectIndex(other);
    key(Qt::Key_Plus);
    key(Qt::Key_Minus);
    const bool otherEmpty=projects.cityLayouts().isEmpty();
    projects.setCurrentProjectIndex(testProject);
    projects.removeProject(other);
    if (!otherEmpty || layout.size()!=1) return 77;
    key(Qt::Key_Plus);
    if (layout.size()!=2) return 78;
    // Do not add another piece on top of a connector already occupied by a track.
    auto *tail=layout.selectedElement();
    auto *blocker=new CityLayoutElement("Blocker",tail->sourcePixmap(),tail->plateSize(),
        LegoGrid::pixelsPerStud,projects.currentProject()->zoomFactor,&view,tail->resourcePath());
    blocker->rotateByDegrees(tail->rotationDegrees());
    blocker->move((QPointF(tail->pos())+tail->trackConnections()[1].p1()
                   -blocker->trackConnections()[0].p1()).toPoint());
    layout.append(blocker);
    const auto occupiedRevision=view.changeRevision();
    key(Qt::Key_Plus);
    if (layout.size()!=3 || view.changeRevision()!=occupiedRevision) return 82;
    layout.deleteImage(blocker);
    key(Qt::Key_Minus);
    key(Qt::Key_Minus);
    for (const QString &path : {QString(":/images/LeftSwitchTrack.png"),QString(":/images/RightSwitchTrack.png")}) {
        auto *sw=drop(path,QSize(36,24),QPoint(500,500));
        if (!sw) return 79;
        const auto before=view.changeRevision();
        key(Qt::Key_Plus);
        if (layout.size()!=1 || view.changeRevision()!=before) return 80;
        key(Qt::Key_Minus);
        if (!layout.isEmpty()) return 81;
    }
    qInfo()<<"PASS: track run +/-; straight, curve, flex, reverse attachment, trimming, layer order, project isolation; switches excluded";
    return 0;
}
