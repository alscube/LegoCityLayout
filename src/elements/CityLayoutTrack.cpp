
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

#include "CityLayoutTrack.h"
#include "CityLayoutElement.h"
#include "LegoGrid.h"

#include <QtMath>
#include <QPainter>
#include <QPixmapCache>
#include <QPainterPath>
#include <QPolygonF>
#include <QRadialGradient>

std::unique_ptr<CityLayoutTrack> CityLayoutTrack::create(const QString &resourcePath)
{
    if (resourcePath != QStringLiteral(":/images/StrightTrack.png")
        && resourcePath != QStringLiteral(":/images/CurvedTrack.png")
        && resourcePath != QStringLiteral(":/images/LeftSwitchTrack.png")
        && resourcePath != QStringLiteral(":/images/RightSwitchTrack.png")
        && resourcePath != QStringLiteral(":/images/FlexTrack.png")) return nullptr;
    return std::unique_ptr<CityLayoutTrack>(new CityLayoutTrack(resourcePath));
}

CityLayoutTrack::CityLayoutTrack(const QString &resourcePath)
{
    if (resourcePath == QStringLiteral(":/images/LeftSwitchTrack.png")) m_switchHandedness = -1;
    else if (resourcePath == QStringLiteral(":/images/RightSwitchTrack.png")) m_switchHandedness = 1;
    m_isFlexTrack = resourcePath == QStringLiteral(":/images/FlexTrack.png");
    m_isStraightTrack = resourcePath == QStringLiteral(":/images/StrightTrack.png");
    m_isCurvedTrack = resourcePath == QStringLiteral(":/images/CurvedTrack.png");
}

void CityLayoutTrack::calibrate(CityLayoutElement &element, qreal pixelsPerStud)
{
    // Refresh saved switches and flex track with current calibrated artwork.
    if (isSwitchTrack() || isFlexTrack()) {
        const QPixmap currentTrack(element.m_resourcePath);
        if (!currentTrack.isNull()) element.m_originalPixmap = currentTrack;
    }
    if (m_isStraightTrack) {
        // Straight artwork uses its native horizontal orientation.
        const QPixmap currentStraight(element.m_resourcePath);
        if (!currentStraight.isNull()) element.m_originalPixmap = currentStraight;
    }
    // Refresh saved curves with the current calibrated artwork.
    QPixmap currentCurve;
    if (m_isCurvedTrack) currentCurve.load(element.m_resourcePath);
    const bool stockCurve = m_isCurvedTrack && !currentCurve.isNull();
    if (stockCurve) element.m_originalPixmap = currentCurve;
    element.m_sourceBounds = element.m_originalPixmap.rect();
    element.m_unscaledFootprint = QRectF(QPointF(), element.m_unscaledSize);
    // Calibrate the bundled StrightTrack artwork by its sleeper body, not its
    // padded PNG canvas. Saved tracks retain this full-resolution source image.
    if (m_isStraightTrack && element.m_originalPixmap.size() == QSize(1377, 1142)) {
        element.m_sourceBounds = QRect(33, 310, 1344, 640);
        constexpr qreal bodyLeft = 50.0;
        constexpr qreal bodyWidth = 1248.0;
        const qreal scaleX = element.m_unscaledSize.width() / bodyWidth;
        element.m_unscaledFootprint = QRectF(bodyLeft * scaleX, 0.0,
                                            element.m_unscaledSize.width(), element.m_unscaledSize.height());
        element.m_unscaledSize.setWidth(element.m_sourceBounds.width() * scaleX);
    }
    if (stockCurve) {
        // Retain horizontal connector margins; the calibrated artwork fits an 11-stud height.
        element.m_unscaledSize = QSizeF(19 * pixelsPerStud, 11 * pixelsPerStud);
        element.m_unscaledFootprint = QRectF(pixelsPerStud, 0,
                                              17 * pixelsPerStud, 11 * pixelsPerStud);
        if (element.m_originalPixmap.size() == QSize(1515, 1038)) {
            // Fit the 12 terminal stud centers to the physical curve geometry.
            // Artwork margins and slightly oversized molded edges are not scale references.
            m_curveArtworkTransform = QTransform(
                0.012558170872749401 * pixelsPerStud,
                -0.000017760299693847134 * pixelsPerStud,
                -0.000011133699827966163 * pixelsPerStud,
                0.012462392679827995 * pixelsPerStud,
                -0.02916372853303343 * pixelsPerStud,
                -0.971507261232889094 * pixelsPerStud);
        }
    }
}

QPixmap CityLayoutTrack::renderArtwork(const CityLayoutElement &element, const QSize &renderSize) const
{
    // Share prepared source artwork between identical tracks, with bounded memory.
    static const bool cacheConfigured = [] {
        QPixmapCache::setCacheLimit(qMax(QPixmapCache::cacheLimit(), 64 * 1024));
        return true;
    }();
    Q_UNUSED(cacheConfigured);
    QPixmap displayedPixmap;
    if (isFlexTrack() && !qFuzzyIsNull(m_flexBendDegrees)) {
        displayedPixmap = QPixmap(renderSize);
        displayedPixmap.fill(Qt::transparent);
        QPainter painter(&displayedPixmap);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QTransform rotation = QTransform().rotate(element.rotationDegrees());
        const QRectF bounds = rotation.mapRect(QRectF(QPointF(), element.m_unscaledSize));
        painter.scale(element.m_zoomFactor * element.devicePixelRatioF(),
                      element.m_zoomFactor * element.devicePixelRatioF());
        painter.translate(-bounds.topLeft());
        painter.rotate(element.rotationDegrees());
        const QRectF canvas(QPointF(), element.m_unscaledSize);
        painter.drawPixmap(canvas, QPixmap(QStringLiteral(":/images/FlexTrackFemale.png")),
                           QRectF(0, 0, 320, 400));
        const QPointF hinge(element.m_unscaledSize.width() / 2, element.m_unscaledSize.height() / 2);
        painter.translate(hinge);
        painter.rotate(m_flexBendDegrees);
        painter.translate(-hinge);
        painter.drawPixmap(canvas, QPixmap(QStringLiteral(":/images/FlexTrackMale.png")),
                           QRectF(0, 0, 320, 400));
    } else if (m_isCurvedTrack && !m_curveArtworkTransform.isIdentity()) {
        displayedPixmap = QPixmap(renderSize);
        displayedPixmap.fill(Qt::transparent);
        QPainter painter(&displayedPixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QTransform rotation = QTransform().rotate(element.rotationDegrees());
        const QRectF bounds = rotation.mapRect(QRectF(QPointF(), element.m_unscaledSize));
        painter.scale(element.m_zoomFactor * element.devicePixelRatioF(),
                      element.m_zoomFactor * element.devicePixelRatioF());
        painter.translate(-bounds.topLeft());
        painter.rotate(element.rotationDegrees());
        // Keep the photo's sleeper body inside the actual eight-stud track width.
        // Replace the terminal sleepers and couplings with the straight track's
        // matching end artwork; retain the calibrated curved body between them.
        const QPixmap straightEnds(QStringLiteral(":/images/StrightTrack.png"));
        const bool matchingEnds = straightEnds.size() == QSize(1377, 1142);
        const qreal startStation = matchingEnds ? 1.0 : -0.8;
        const qreal endStation = matchingEnds ? 15.0 : 16.8;
        const qreal pitch = element.m_unscaledFootprint.width() / 17.0;
        QPolygonF corridor;
        const auto corridorPoint = [&](qreal station, qreal normal) {
            const qreal angle = station / LegoGrid::curveRadiusStuds;
            return QPointF(1 + (LegoGrid::curveRadiusStuds + normal) * std::sin(angle),
                           4 + LegoGrid::curveRadiusStuds
                             - (LegoGrid::curveRadiusStuds + normal) * std::cos(angle)) * pitch;
        };
        for (int index = 0; index <= 64; ++index)
            corridor.append(corridorPoint(startStation + (endStation - startStation) * index / 64.0, 4.0));
        for (int index = 64; index >= 0; --index)
            corridor.append(corridorPoint(startStation + (endStation - startStation) * index / 64.0, -4.0));
        QPainterPath clip;
        clip.addPolygon(corridor);
        clip.closeSubpath();
        painter.save();
        painter.setClipPath(clip);
        painter.setTransform(m_curveArtworkTransform, true);
        painter.drawPixmap(QPointF(), QPixmap(QStringLiteral(":/images/CurvedTrack.png")));
        painter.restore();
        if (matchingEnds) {
            // Use the same eight-stud width and 78 source pixels per lengthwise
            // stud as the calibrated straight piece. Couplings straddle the seam.
            const auto drawEnd = [&](const QPointF &center, qreal angle,
                                     const QRectF &source, qreal seamX) {
                painter.save();
                painter.translate(center * pitch);
                painter.rotate(angle);
                painter.drawPixmap(QRectF((source.x() - seamX) / 78.0 * pitch, -4 * pitch,
                                          source.width() / 78.0 * pitch, 8 * pitch),
                                   straightEnds, source);
                painter.restore();
            };
            drawEnd(QPointF(1, 4), 0.0, QRectF(33, 310, 135, 640), 83.0);
            const qreal sweep = qDegreesToRadians(LegoGrid::curveSweepDegrees);
            drawEnd(QPointF(1 + LegoGrid::curveRadiusStuds * std::sin(sweep),
                           4 + LegoGrid::curveRadiusStuds * (1 - std::cos(sweep))),
                    LegoGrid::curveSweepDegrees,
                    QRectF(1245, 310, 132, 640), 1331.0);
        }
    } else {
        const QString key = QStringLiteral("track-source:%1:%2,%3,%4,%5:%6")
            .arg(element.m_originalPixmap.cacheKey())
            .arg(element.m_sourceBounds.x()).arg(element.m_sourceBounds.y())
            .arg(element.m_sourceBounds.width()).arg(element.m_sourceBounds.height()).arg(m_isStraightTrack);
        QPixmap artwork;
        if (!QPixmapCache::find(key, &artwork)) {
            QPixmap source = element.m_originalPixmap.copy(element.m_sourceBounds);
            if (m_isStraightTrack) {
                // Mounting holes sit between the stud rows at the center of each
                // large sleeper. Render them before rotation so they follow the track.
                QPainter holes(&source);
                holes.setRenderHint(QPainter::Antialiasing);
                for (qreal x : {364.0, 674.0, 984.0}) {
                    const QPointF center(x, source.height() / 2.0);
                    QRadialGradient bevel(center, 23.0, center + QPointF(6, -6));
                    bevel.setColorAt(0.0, QColor(65, 67, 69));
                    bevel.setColorAt(0.72, QColor(140, 143, 146));
                    bevel.setColorAt(1.0, QColor(55, 57, 59));
                    holes.setCompositionMode(QPainter::CompositionMode_SourceOver);
                    holes.setPen(Qt::NoPen);
                    holes.setBrush(bevel);
                    holes.drawEllipse(center, 23.0, 23.0);
                    holes.setCompositionMode(QPainter::CompositionMode_Clear);
                    holes.setBrush(Qt::black);
                    holes.drawEllipse(center, 17.0, 17.0);
                }
            }
            artwork = source;
            QPixmapCache::insert(key, artwork);
        }
        displayedPixmap = QPixmap(renderSize);
        displayedPixmap.fill(Qt::transparent);
        QPainter painter(&displayedPixmap);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QTransform rotation = QTransform().rotate(element.rotationDegrees());
        const QRectF bounds = rotation.mapRect(QRectF(QPointF(), element.m_unscaledSize));
        painter.scale(element.m_zoomFactor * element.devicePixelRatioF(),
                      element.m_zoomFactor * element.devicePixelRatioF());
        painter.translate(-bounds.topLeft());
        painter.rotate(element.rotationDegrees());
        painter.drawPixmap(QRectF(QPointF(), element.m_unscaledSize), artwork, QRectF(artwork.rect()));
    }
    return displayedPixmap;
}

void CityLayoutTrack::updateSelectionBounds(const QPixmap &displayedPixmap, const QSize &size)
{
    // Measure the rendered artwork, excluding transparent canvas added by rotation.
    const QImage selectionImage = displayedPixmap.toImage().scaled(size, Qt::IgnoreAspectRatio,
        Qt::FastTransformation).convertToFormat(QImage::Format_ARGB32);
    m_selectionBounds = QRect();
    for (int y = 0; y < size.height(); ++y) {
        const auto *row = reinterpret_cast<const QRgb *>(selectionImage.constScanLine(y));
        int first = -1;
        int last = -1;
        for (int x = 0; x < size.width(); ++x) {
            if (qAlpha(row[x]) >= 32) {
                if (first < 0) first = x;
                last = x;
            }
        }
        if (first >= 0) m_selectionBounds = m_selectionBounds.united(QRect(first, y, last - first + 1, 1));
    }
}

bool CityLayoutTrack::containsPoint(const QRect &rect, const QPoint &point) const
{
    // Include holes and gaps so selecting a track never clicks through to a plate.
    return m_selectionBounds.adjusted(-2, -2, 2, 2).intersected(rect).contains(point);
}

void CityLayoutTrack::paintSelection(CityLayoutElement &element) const
{
    if (!element.m_selectionHighlighted
        || m_selectionBounds.isEmpty()) return;
    QPainter painter(&element);
    painter.setPen(QPen(QColor("orange"), 2));
    painter.setBrush(Qt::NoBrush);
    const QRect outline = m_selectionBounds.adjusted(-2, -2, 2, 2)
        .intersected(element.rect().adjusted(1, 1, -1, -1));
    painter.drawRect(outline);
}

qreal CityLayoutTrack::rotationStepDegrees() const
{
    return isFlexTrack() ? LegoGrid::flexMaximumBendDegrees : LegoGrid::curveSweepDegrees;
}

bool CityLayoutTrack::setFlexBendDegrees(qreal degrees)
{
    if (!isFlexTrack() || !std::isfinite(degrees)
        || qAbs(degrees) > LegoGrid::flexMaximumBendDegrees
        || qFuzzyCompare(degrees + 1.0, m_flexBendDegrees + 1.0)) return false;
    m_flexBendDegrees = degrees;
    return true;
}

QPointF CityLayoutTrack::displayedPoint(const CityLayoutElement &element, const QPointF &point) const
{
    const QTransform rotation = QTransform().rotate(element.rotationDegrees());
    const QRectF bounds = rotation.mapRect(QRectF(QPointF(), element.m_unscaledSize));
    const QPointF rotated = rotation.map(point) - bounds.topLeft();
    // Never derive physical geometry from rounded raster dimensions: unequal
    // x/y rounding changes connector angles and accumulates around a curve.
    return rotated * element.m_zoomFactor;
}

QList<QLineF> CityLayoutTrack::connections(const CityLayoutElement &element) const
{
    QList<QLineF> result;
    const qreal pitch = element.m_unscaledFootprint.width() / element.m_plateSize.width();
    const auto addConnection = [&](const QPointF &studPoint, qreal outwardDegrees) {
        const QPointF point = element.m_unscaledFootprint.topLeft() + studPoint * pitch;
        const qreal angle = qDegreesToRadians(outwardDegrees);
        const QPointF direction(std::cos(angle), std::sin(angle));
        result.append(QLineF(displayedPoint(element, point), displayedPoint(element, point + direction * pitch)));
    };
    if (isFlexTrack()) {
        const qreal bend = qDegreesToRadians(m_flexBendDegrees);
        addConnection(QPointF(2, 5), 180);
        addConnection(QPointF(4 + 2 * std::cos(bend), 5 + 2 * std::sin(bend)), m_flexBendDegrees);
    } else if (isSwitchTrack()) {
        // Each image keeps its native horizontal direction and mirrored entrance.
        const qreal trunkY = LegoGrid::switchCanvasHeightStuds - 4.0;
        const qreal entranceX = m_switchHandedness > 0
            ? LegoGrid::switchEntranceInsetStuds
            : LegoGrid::switchCanvasWidthStuds - LegoGrid::switchEntranceInsetStuds;
        const qreal exitAngle = m_switchHandedness > 0 ? 0.0 : 180.0;
        addConnection(QPointF(entranceX, trunkY), exitAngle + 180.0);
        addConnection(QPointF(entranceX + m_switchHandedness * LegoGrid::switchStraightLengthStuds,
                             trunkY), exitAngle);
        addConnection(QPointF(entranceX + m_switchHandedness * LegoGrid::switchBranchLengthStuds,
                             trunkY - LegoGrid::switchBranchOffsetStuds),
                      exitAngle - m_switchHandedness * LegoGrid::curveSweepDegrees);
    } else if (m_isStraightTrack) {
        addConnection(QPointF(0, 4), 180);
        addConnection(QPointF(16, 4), 0);
    } else {
        const qreal angle = qDegreesToRadians(LegoGrid::curveSweepDegrees);
        addConnection(QPointF(0, 4), 180);
        addConnection(QPointF(LegoGrid::curveRadiusStuds * std::sin(angle),
                             4 + LegoGrid::curveRadiusStuds * (1 - std::cos(angle))),
                      LegoGrid::curveSweepDegrees);
    }
    return result;
}
