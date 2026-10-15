
// Copyright 2026. Alan Krzywicki

// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
#pragma once

#include <QHash>
#include <QPixmap>
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
inline constexpr qreal curveSweepDegrees = 22.5;
// Standard LEGO R40 curves have a 40-stud centerline radius. Their nominal
// 16-stud length is not the exact arc length (40 * pi / 8).
inline constexpr qreal curveRadiusStuds = 40.0;

// Horizontal switch canvases. Connector dimensions measured from LDraw
// 53404/53407 (20 LDU per stud); the entrance is inset from either side.
inline constexpr qreal switchCanvasWidthStuds = 36.0;
inline constexpr qreal switchCanvasHeightStuds = 24.0;
inline constexpr qreal switchEntranceInsetStuds = 1.0;
inline constexpr qreal switchStraightLengthStuds = 32.0;
inline constexpr qreal switchBranchOffsetStuds = 259.103 / 20.0;
inline constexpr qreal switchBranchLengthStuds = (333.852 + 320.0) / 20.0;

inline constexpr qreal flexLengthStuds = 4.0;
inline constexpr qreal flexMaximumBendDegrees = 5.625;
inline constexpr qreal flexCanvasWidthStuds = 8.0;
inline constexpr qreal flexCanvasHeightStuds = 10.0;

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
    // Cache tiles in grid coordinates so translating the view reuses the studs.
    // Geometry checks and antialiased ellipse rendering run only for new tiles.
    struct StudCache {
        QPainterPath surface;
        qreal zoom = 0;
        qreal devicePixelRatio = 0;
        QHash<QPair<int, int>, QPixmap> tiles;
    };
    static StudCache cache;
    const QPainterPath localSurface = QTransform::fromTranslate(-origin.x(), -origin.y()).map(surface);
    const qreal dpr = painter.device()->devicePixelRatioF();
    bool sameSurface = cache.surface.fillRule() == localSurface.fillRule()
        && cache.surface.elementCount() == localSurface.elementCount();
    for (int i = 0; sameSurface && i < localSurface.elementCount(); ++i) {
        const auto previous = cache.surface.elementAt(i);
        const auto current = localSurface.elementAt(i);
        sameSurface = previous.type == current.type
            && qAbs(previous.x - current.x) < 1e-6
            && qAbs(previous.y - current.y) < 1e-6;
    }
    if (!sameSurface || !qFuzzyCompare(cache.zoom, zoom)
        || !qFuzzyCompare(cache.devicePixelRatio, dpr)) {
        cache.surface = localSurface;
        cache.zoom = zoom;
        cache.devicePixelRatio = dpr;
        cache.tiles.clear();
    }
    constexpr int tileSize = 256;
    const QRectF localBounds = bounds.translated(-origin);
    const int left = int(std::floor(localBounds.left() / tileSize));
    const int right = int(std::floor(localBounds.right() / tileSize));
    const int top = int(std::floor(localBounds.top() / tileSize));
    const int bottom = int(std::floor(localBounds.bottom() / tileSize));
    painter.save();
    for (int row = top; row <= bottom; ++row) {
        for (int column = left; column <= right; ++column) {
            const QPair<int, int> key(column, row);
            auto tile = cache.tiles.constFind(key);
            if (tile == cache.tiles.cend()) {
                // Bound retained raster memory while exploring large layouts.
                if (cache.tiles.size() >= 64) cache.tiles.erase(cache.tiles.begin());
                QPixmap image(qCeil(tileSize * dpr), qCeil(tileSize * dpr));
                image.setDevicePixelRatio(dpr);
                image.fill(Qt::transparent);
                QPainter tilePainter(&image);
                tilePainter.setRenderHint(QPainter::Antialiasing);
                tilePainter.translate(-column * tileSize, -row * tileSize);
                tilePainter.setPen(QPen(QColor(80, 55, 30, 100), 0.75));
                tilePainter.setBrush(QColor(255, 255, 255, 35));
                const QRectF tileBounds(column * tileSize, row * tileSize, tileSize, tileSize);
                const QRectF centers = tileBounds.adjusted(-radius - 1, -radius - 1, radius + 1, radius + 1);
                const qreal firstX = (std::ceil(centers.left() / spacing - 0.5) + 0.5) * spacing;
                const qreal firstY = (std::ceil(centers.top() / spacing - 0.5) + 0.5) * spacing;
                for (qreal y = firstY; y <= centers.bottom(); y += spacing) {
                    for (qreal x = firstX; x <= centers.right(); x += spacing) {
                        // Keep complete studs away from table edges and holes.
                        const qreal extent = radius + 1.5 + 0.75 / 2.0;
                        if (cache.surface.contains(QRectF(x - extent, y - extent, 2 * extent, 2 * extent)))
                            tilePainter.drawEllipse(QPointF(x, y), radius, radius);
                    }
                }
                tilePainter.end();
                cache.tiles.insert(key, image);
                tile = cache.tiles.constFind(key);
            }
            painter.drawPixmap(QPointF(column * tileSize, row * tileSize) + origin, tile.value());
        }
    }
    painter.restore();
}
}
