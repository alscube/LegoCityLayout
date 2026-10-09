#pragma once

#include <QPainter>
#include <QPainterPath>
#include <QtMath>
#include <cmath>

namespace LegoGrid {
inline constexpr qreal millimetersPerInch = 25.4;
inline constexpr qreal studPitchMillimeters = 8.0;
inline constexpr qreal studDiameterMillimeters = 4.8;
// Preserve the table editor's physical canvas scale.
inline constexpr qreal pixelsPerInch = 6.4;
inline constexpr qreal pixelsPerStud = pixelsPerInch * studPitchMillimeters / millimetersPerInch;
inline constexpr qreal plateStuds = 32.0;
inline constexpr qreal plateSpacing = plateStuds * pixelsPerStud;
inline constexpr qreal curveArcStuds = 16.0;
inline constexpr qreal curveSweepDegrees = 22.5;
inline constexpr qreal curveRadiusStuds = curveArcStuds / (3.14159265358979323846 / 8.0);

inline QPointF snappedPoint(const QPointF &point, const QPointF &origin, qreal zoom)
{
    const qreal spacing = pixelsPerStud * zoom;
    const QPointF relative = point - origin;
    return origin + QPointF(qRound(relative.x() / spacing) * spacing,
                            qRound(relative.y() / spacing) * spacing);
}

inline void paintStuds(QPainter &painter, const QPainterPath &surface,
                      const QRectF &viewport, const QPointF &origin, qreal zoom)
{
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
}
