#pragma once

#include <QEventLoop>
#include <QWheelEvent>
#include <QTimer>

static int checkZoom(LoadedProjects &projects)
{
    const int original = projects.currentProjectIndex();
    const int index = projects.addProject("Zoom checks");
    projects.setCurrentProjectIndex(index);
    CityLayoutView view;
    struct Cleanup {
        LoadedProjects &projects;
        int index, original;
        ~Cleanup() { projects.removeProject(index); projects.setCurrentProjectIndex(original); }
    } cleanup{projects,index,original};
    const QString path=":/images/StrightTrack.png";
    auto *track=new CityLayoutElement("Track",QPixmap(path),QSize(16,8),LegoGrid::pixelsPerStud,
                                     projects.currentProject()->zoomFactor,&view,path);
    projects.cityLayouts().append(track);
    track->move(201,309);
    const QPoint originalPosition=track->pos();
    const qreal originalZoom=projects.currentProject()->zoomFactor;
    const QPointF anchor(33.25,97.75);
    for(int step=0;step<50;++step) projects.setViewZoom(anchor,projects.currentProject()->zoomFactor*1.01);
    for(int step=0;step<50;++step) projects.setViewZoom(anchor,projects.currentProject()->zoomFactor/1.01);
    if(track->pos()!=originalPosition || !near(projects.currentProject()->zoomFactor,originalZoom)) return 90;
    track->move(123,456);
    projects.setViewZoom(anchor,originalZoom*1.02);
    projects.setViewZoom(anchor,originalZoom);
    if(track->pos()!=QPoint(123,456)) return 91;
    const auto wheel=[&](QPoint pixels,QPoint angle) {
        QWheelEvent event(anchor,anchor,pixels,angle,Qt::NoButton,Qt::NoModifier,Qt::NoScrollPhase,false);
        QApplication::sendEvent(&view,&event);
    };
    const auto settle=[] {
        QEventLoop loop;
        QTimer::singleShot(250,&loop,&QEventLoop::quit);
        loop.exec();
    };
    for(int event=0;event<25;++event) wheel(QPoint(),QPoint(0,4));
    if(!near(projects.currentProject()->zoomFactor,originalZoom)) return 92;
    settle();
    if(!near(projects.currentProject()->zoomFactor,originalZoom*std::pow(1.0015,100))) return 93;
    for(int event=0;event<25;++event) wheel(QPoint(),QPoint(0,-4));
    settle();
    if(track->pos()!=QPoint(123,456) || !near(projects.currentProject()->zoomFactor,originalZoom)) return 94;
    wheel(QPoint(0,10),QPoint());
    settle();
    if(!near(projects.currentProject()->zoomFactor,originalZoom*std::pow(1.0015,20))) return 95;
    const qreal before=projects.currentProject()->zoomFactor;
    wheel(QPoint(),QPoint(0,120));
    view.activateProject();
    settle();
    if(!near(projects.currentProject()->zoomFactor,before)) return 96;
    // Settled rendering must match a canvas with a freshly generated stud layer.
    track->hide();
    projects.setViewZoom(anchor,4.0);
    projects.tableDefinition().addSurface(TableSurface("Zoom table",QPolygonF(QRectF(80,60,400,320))));
    view.resize(640,480);
    view.grab();
    wheel(QPoint(),QPoint(0,120));
    settle();
    CityLayoutView fresh;
    fresh.resize(view.size());
    if(view.grab().toImage()!=fresh.grab().toImage()) return 97;
    projects.currentProject()->gridOrigin+=QPointF(12,-7);
    projects.tableDefinition().translate(QPointF(12,-7));
    if(view.grab().toImage()!=fresh.grab().toImage()) return 98;
    qInfo()<<"PASS: fractional zoom positions, wheel coalescing, pixel wheel input, pending-zoom cancellation";
    return 0;
}
