#pragma once

#include "LegoGrid.h"
#include "TableDefinition.h"
#include <QElapsedTimer>
#include <QImage>
#include <QDebug>

inline void paintReferenceStuds(QPainter &painter, const QPainterPath &surface,
                      const QRectF &viewport, const QPointF &origin, qreal zoom)
{
    using namespace LegoGrid;
    const qreal spacing = pixelsPerStud * zoom;
    // Individual studs become visible as the user zooms in.
    if (spacing < 4.0 || surface.isEmpty()) return;
    const qreal radius = spacing * studDiameterMillimeters / studPitchMillimeters / 2.0;
    const QRectF bounds = surface.boundingRect().intersected(viewport).adjusted(-radius, -radius, radius, radius);
    if (bounds.isEmpty()) return;
    // Grid lines mark cell edges; stud centers sit half a pitch inside each cell.
    const qreal firstX = origin.x() + (std::ceil((bounds.left() - origin.x()) / spacing - 0.5) + 0.5) * spacing;
    const qreal firstY = origin.y() + (std::ceil((bounds.top() - origin.y()) / spacing - 0.5) + 0.5) * spacing;
    painter.save();
    painter.setClipPath(surface, Qt::IntersectClip);
    painter.setPen(QPen(QColor(80, 55, 30, 100), 0.75));
    painter.setBrush(QColor(255, 255, 255, 35));
    for (qreal y = firstY; y <= bounds.bottom(); y += spacing) {
        for (qreal x = firstX; x <= bounds.right(); x += spacing) {
            // A table boundary may fall between stud cells in an existing layout.
            // Leave room for the 3-pixel table outline and the stud's own stroke;
            // paint complete studs rather than clipped fragments at edges/holes.
            constexpr qreal outlineClearance = 1.5 + 0.75 / 2.0;
            const qreal extent = radius + outlineClearance;
            const QRectF studBounds(x - extent, y - extent, 2 * extent, 2 * extent);
            if (surface.contains(studBounds)) {
                painter.drawEllipse(QPointF(x, y), radius, radius);
            }
        }
    }
    painter.restore();
}
inline bool checkPanningRendering()
{
    QPainterPath surface;
    surface.addRect(QRectF(-400, -300, 2600, 1900));
    QPainterPath hole;
    hole.addRect(QRectF(380, 240, 200, 180));
    surface = surface.subtracted(hole);
    const QSize size(960, 640);
    const auto render = [&](bool cached, QPointF origin, qreal zoom, qreal dpr) {
        QImage image(size * dpr, QImage::Format_ARGB32_Premultiplied);
        image.setDevicePixelRatio(dpr);
        image.fill(Qt::white);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        const auto moved = QTransform::fromTranslate(origin.x(), origin.y()).map(surface);
        if (cached) LegoGrid::paintStuds(painter, moved, QRectF(QPointF(), size), origin, zoom);
        else paintReferenceStuds(painter, moved, QRectF(QPointF(), size), origin, zoom);
        return image;
    };
    for (qreal zoom : {2.0, 4.0, 12.0}) for (qreal dpr : {1.0, 2.0}) {
        for (QPointF origin : {QPointF(), QPointF(31, -19), QPointF(-283, 271)}) {
            const QImage expected = render(false, origin, zoom, dpr);
            const QImage actual = render(true, origin, zoom, dpr);
            int differences = 0;
            for (int y=0; y<actual.height(); ++y) for (int x=0; x<actual.width(); ++x) {
                const auto a=actual.pixelColor(x,y), b=expected.pixelColor(x,y);
                if (qAbs(a.red()-b.red())>3 || qAbs(a.green()-b.green())>3 || qAbs(a.blue()-b.blue())>3)
                    ++differences;
            }
            if (differences > actual.width()*actual.height()/1000) {
                qWarning() << "Stud cache rendering differs" << zoom << dpr << origin << differences;
                return false;
            }
        }
    }
    QElapsedTimer timer;
    timer.start();
    for (int i=0;i<30;++i) render(false,QPointF(i,0),2.0,1.0);
    const qint64 reference=timer.elapsed();
    render(true,QPointF(),2.0,1.0);
    timer.restart();
    for (int i=0;i<30;++i) render(true,QPointF(i,0),2.0,1.0);
    qInfo() << "Stud panning, 30 frames: original" << reference << "ms, cached" << timer.elapsed() << "ms";
    TableDefinition definition;
    definition.addSurface(TableSurface("A",QPolygonF(QRectF(0,0,100,100))));
    definition.usableArea();
    definition.translate(QPointF(50,20));
    if (!definition.contains(QPointF(100,70)) || definition.contains(QPointF(10,10))) return false;
    definition.scale(QPointF(),2);
    if (definition.usableArea().boundingRect()!=QRectF(100,40,200,200)) return false;
    definition.setSurface(0,TableSurface("B",QPolygonF(QRectF(0,0,40,40))));
    if (definition.usableArea().boundingRect()!=QRectF(0,0,40,40)) return false;
    definition.addSurface(TableSurface("C",QPolygonF(QRectF(80,0,40,40))));
    if (definition.usableArea().boundingRect()!=QRectF(0,0,120,40)) return false;
    definition.removeSurface(1);
    if (definition.usableArea().boundingRect()!=QRectF(0,0,40,40)) return false;
    definition.clear();
    return definition.usableArea().isEmpty();
}
