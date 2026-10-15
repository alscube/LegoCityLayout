#pragma once

static int checkFlexTrack(CityLayoutView &view, LoadedProjects &projects)
{
    const qreal originalZoom=projects.currentProject()->zoomFactor;
    const QString path=":/images/FlexTrack.png";
    auto *flex=new CityLayoutElement("Flex Track",QPixmap(path),QSize(8,10),LegoGrid::pixelsPerStud,
                                     originalZoom,&view,path);
    projects.cityLayouts().append(flex);
    if (!flex->isFlexTrack() || flex->trackConnections().size()!=2 || flex->rotationStepDegrees()!=5.625) return 30;
    if (flex->setFlexBendDegrees(6.0) || flex->setFlexBendDegrees(-6.0)) return 31;
    const std::array<QString,5> targets{":/images/StrightTrack.png",":/images/CurvedTrack.png",
        ":/images/LeftSwitchTrack.png",":/images/RightSwitchTrack.png",path};
    int connections=0;
    for (qreal zoom : {1.875,4.0,8.0}) {
        projects.currentProject()->zoomFactor=zoom;
        flex->setZoomFactor(zoom);
        for (int type=0;type<5;++type) {
            const QSize size=type==0 ? QSize(16,8) : type==1 ? QSize(17,11) : type==4 ? QSize(8,10) : QSize(36,24);
            auto *target=new CityLayoutElement("Target",QPixmap(targets[type]),size,LegoGrid::pixelsPerStud,zoom,&view,targets[type]);
            if (target->isFlexTrack()) target->setFlexBendDegrees(-5.625);
            target->rotateByDegrees(22.5);
            target->move(1000,1000);
            projects.cityLayouts().append(target);
            for (qreal bend : {-5.625,0.0,5.625}) {
                flex->rotateByDegrees(-flex->rotationDegrees());
                const QPointF before=flex->trackConnections().first().p1();
                flex->setFlexBendDegrees(bend);
                if (QLineF(before,flex->trackConnections().first().p1()).length()>1e-6) return 32;
                if (!near(flex->flexBendDegrees(),bend)) return 33;
                for (const auto &port : target->trackConnections()) for (int end=0;end<2;++end) {
                    const QPointF direction=port.p2()-port.p1();
                    const qreal angle=qRadiansToDegrees(std::atan2(direction.y(),direction.x()));
                    const qreal turn=qRound((angle+180-(end==0 ? 180 : bend))/5.625)*5.625;
                    flex->rotateByDegrees(turn-flex->rotationDegrees());
                    const QLineF moving=flex->trackConnections().at(end);
                    const QPointF ideal=target->layoutPosition()+port.p1()-moving.p1();
                    const QPointF snapped=view.snappedPosition(ideal.toPoint()+QPoint(3,-2),flex);
                    if (QLineF(QPointF(snapped)+moving.p1(),target->layoutPosition()+port.p1()).length()>0.75) return 34;
                    ++connections;
                }
            }
            projects.cityLayouts().deleteImage(target);
        }
    }
    projects.currentProject()->zoomFactor=originalZoom;
    flex->setZoomFactor(originalZoom);
    flex->setFlexBendDegrees(1.25);
    flex->rotateByDegrees(11.25-flex->rotationDegrees());
    flex->move(1000,1000);
    // Render the joint in both directions, alongside the neutral assembly.
    QImage preview(720,440,QImage::Format_ARGB32);
    preview.fill(QColor(229,206,181));
    QPainter painter(&preview);
    for (int i=0;i<3;++i) {
        CityLayoutElement sample("Flex",QPixmap(path),QSize(8,10),24.0,1.0,nullptr,path);
        sample.setFlexBendDegrees((i-1)*5.625);
        painter.drawPixmap(QPoint(20+i*240,50),sample.pixmap());
        painter.drawText(QPoint(25+i*240,325),i==0 ? "Left 5.625 degrees" : i==1 ? "Straight" : "Right 5.625 degrees");
    }
    painter.end();
    preview.save("flex-track-preview.png");
    qInfo()<<"PASS:"<<connections<<"flex connections; entry fixed while bending; angle limits and rendered joint";
    return 0;
}
