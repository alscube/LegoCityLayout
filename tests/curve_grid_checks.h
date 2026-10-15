#pragma once

static int checkCurveGrid(LoadedProjects &projects)
{
    const int original = projects.currentProjectIndex();
    const int index = projects.addProject("Curve grid checks");
    projects.setCurrentProjectIndex(index);
    CityLayoutView view;
    struct Cleanup {
        LoadedProjects &projects; int index, original;
        ~Cleanup() { projects.removeProject(index); projects.setCurrentProjectIndex(original); }
    } cleanup{projects,index,original};
    const QString path = ":/images/CurvedTrack.png";
    for (qreal zoom : {1.875, 4.0, 12.0}) for (qreal rotation : {0.0, 90.0, 180.0, 270.0}) {
        auto &layout = projects.cityLayouts();
        projects.currentProject()->zoomFactor = zoom;
        auto *seed = new CityLayoutElement("Curve",QPixmap(path),QSize(17,11),
            LegoGrid::pixelsPerStud,zoom,&view,path);
        seed->rotateByDegrees(rotation);
        seed->move(300,300);
        layout.append(seed);
        layout.setSelectedElement(seed);
        TrackRun run;
        if (!run.recordAdded(seed,layout)) return 120;
        const QPointF start = seed->layoutPosition() + seed->trackConnections()[0].p1();
        for (int count = 1; count <= 16; ++count) {
            auto *tail = layout.selectedElement();
            const qreal expectedAngle = std::fmod(rotation + (count-1)*22.5,360.0);
            if (!near(tail->rotationDegrees(),expectedAngle)) {
                qCritical()<<"Curve angle drift"<<zoom<<count<<tail->rotationDegrees()<<expectedAngle;
                return 121;
            }
            if (count%4 == 0) {
                const qreal sweep = qDegreesToRadians(count*22.5);
                const QPointF offset = QTransform().rotate(rotation).map(QPointF(
                    40*std::sin(sweep),40*(1-std::cos(sweep))))*LegoGrid::pixelsPerStud*zoom;
                const QPointF end = tail->layoutPosition() + tail->trackConnections()[1].p1();
                // Logical joints and integer-stud quarter turns must stay exact.
                if (QLineF(end,start+offset).length()>1e-6) {
                    qCritical()<<"Curve endpoint drift"<<zoom<<count<<end-(start+offset);
                    return 122;
                }
            }
            if (zoom == 12.0 && rotation == 0.0 && (count == 4 || count == 8)) {
                const QPointF endpoint = tail->layoutPosition()+tail->trackConnections()[1].p1();
                const QRectF viewport(endpoint-QPointF(280,280),QSizeF(700,560));
                QImage preview(700,560,QImage::Format_ARGB32_Premultiplied);
                preview.fill(QColor(229,206,181));
                QPainter painter(&preview);
                painter.translate(-viewport.topLeft());
                QPainterPath surface;
                surface.addRect(viewport.adjusted(-100,-100,100,100));
                const QPointF origin = seed->layoutPosition()+seed->footprintRect().topLeft();
                LegoGrid::paintStuds(painter,surface,viewport,origin,zoom);
                for (auto *piece : layout) painter.drawPixmap(piece->pos(),piece->pixmap());
                painter.end();
                if (!preview.save(QString("curve-grid-%1-pieces.png").arg(count))) return 124;
            }
            if (count<16 && !run.extend(layout,zoom,&view)) return 123;
        }
        while (!layout.isEmpty()) layout.deleteImage(layout.last());
    }
    qInfo()<<"PASS: quarter, half, and full curves align to the R40 stud geometry at three zoom levels in four orientations";
    return 0;
}
