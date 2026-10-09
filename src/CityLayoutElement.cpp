
#include "CityLayoutElement.h"
#include "LegoGrid.h"

#include <QtMath>
#include <QTransform>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QRadialGradient>
#include <QImage>
#include <array>

namespace {
struct TrackColor {
    std::array<double, 3> mean{};
    std::array<double, 3> deviation{};
};

TrackColor trackColor(const QImage &image)
{
    TrackColor result;
    std::array<double, 3> squares{};
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        const auto *row = reinterpret_cast<const QRgb *>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb pixel = row[x];
            if (qAlpha(pixel) < 240) continue;
            const std::array<double, 3> channels{double(qRed(pixel)), double(qGreen(pixel)), double(qBlue(pixel))};
            ++count;
            for (int channel = 0; channel < 3; ++channel) {
                result.mean[channel] += channels[channel];
                squares[channel] += channels[channel] * channels[channel];
            }
        }
    }
    if (count == 0) return result;
    for (int channel = 0; channel < 3; ++channel) {
        result.mean[channel] /= count;
        result.deviation[channel] = std::sqrt(qMax(0.0, squares[channel] / count
            - result.mean[channel] * result.mean[channel]));
    }
    return result;
}

const QPixmap &matchingCurveArtwork()
{
    // Match the stock photos' plastic tone and contrast once, retaining their
    // shading, stud detail, and transparent cutouts. Source assets stay intact.
    static const QPixmap matched = [] {
        QImage curve(QStringLiteral(":/images/CurvedTrack.png"));
        QImage straight(QStringLiteral(":/images/StrightTrack.png"));
        if (curve.isNull() || straight.size() != QSize(1142, 1377)) return QPixmap::fromImage(curve);
        curve = curve.convertToFormat(QImage::Format_ARGB32);
        straight = straight.copy(310, 0, 640, 1344).convertToFormat(QImage::Format_ARGB32);
        const TrackColor from = trackColor(curve);
        const TrackColor to = trackColor(straight);
        for (int y = 0; y < curve.height(); ++y) {
            auto *row = reinterpret_cast<QRgb *>(curve.scanLine(y));
            for (int x = 0; x < curve.width(); ++x) {
                const QRgb pixel = row[x];
                if (qAlpha(pixel) == 0) continue;
                const std::array<int, 3> channels{qRed(pixel), qGreen(pixel), qBlue(pixel)};
                std::array<int, 3> adjusted{};
                for (int channel = 0; channel < 3; ++channel) {
                    const double scale = from.deviation[channel] > 0.0
                        ? to.deviation[channel] / from.deviation[channel] : 1.0;
                    adjusted[channel] = qBound(0, qRound(to.mean[channel]
                        + (channels[channel] - from.mean[channel]) * scale), 255);
                }
                row[x] = qRgba(adjusted[0], adjusted[1], adjusted[2], qAlpha(pixel));
            }
        }
        return QPixmap::fromImage(curve);
    }();
    return matched;
}
}

CityLayoutElement::CityLayoutElement(const QString &name,
                                     const QPixmap &pixmap,
                                     const QSize &plateSize,
                                     qreal pixelsPerStud,
                                     qreal zoomFactor,
                                     QWidget *parent)
    : QLabel(parent), m_name(name), m_originalPixmap(pixmap)
    , m_plateSize(plateSize)
    , m_unscaledSize(plateSize.width() * pixelsPerStud,
                     plateSize.height() * pixelsPerStud)
{
    // Refresh recognized stock curves embedded in saved layouts with current art.
    QPixmap currentCurve;
    if (plateSize == QSize(17, 11)) currentCurve.load(QStringLiteral(":/images/CurvedTrack.png"));
    const bool stockCurve = plateSize == QSize(17, 11) && !currentCurve.isNull()
        && (pixmap.size() == QSize(1520, 1040) || pixmap.size() == QSize(1516, 1038)
            || pixmap.size() == currentCurve.size());
    if (stockCurve) m_originalPixmap = currentCurve;
    m_sourceBounds = m_originalPixmap.rect();
    m_unscaledFootprint = QRectF(QPointF(), m_unscaledSize);
    // Calibrate the bundled StrightTrack artwork by its sleeper body, not its
    // padded PNG canvas. Saved tracks retain this full-resolution source image.
    if (plateSize == QSize(8, 16) && pixmap.size() == QSize(1142, 1377)) {
        m_sourceBounds = QRect(310, 0, 640, 1344);
        constexpr qreal bodyTop = 46.0;
        constexpr qreal bodyHeight = 1248.0;
        const qreal scaleY = m_unscaledSize.height() / bodyHeight;
        m_unscaledFootprint = QRectF(0.0, bodyTop * scaleY,
                                    m_unscaledSize.width(), m_unscaledSize.height());
        m_unscaledSize.setHeight(m_sourceBounds.height() * scaleY);
        m_isStraightTrack = true;
    }
    if (stockCurve) {
        // Retain horizontal connector margins; the calibrated artwork fits an 11-stud height.
        m_isCurvedTrack = true;
        m_unscaledSize = QSizeF(19 * pixelsPerStud, 11 * pixelsPerStud);
        m_unscaledFootprint = QRectF(pixelsPerStud, 0,
                                    17 * pixelsPerStud, 11 * pixelsPerStud);
        if (m_originalPixmap.size() == QSize(1515, 1038)) {
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
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setZoomFactor(zoomFactor);
}


QString CityLayoutElement::name() const
{
    return m_name;
}


void CityLayoutElement::setZoomFactor(qreal zoomFactor)
{
    m_zoomFactor = zoomFactor;
    const QSizeF rotatedSize = QTransform().rotate(rotationDegrees())
        .mapRect(QRectF(QPointF(), m_unscaledSize)).size();
    const QSize size(qMax(1, qRound(rotatedSize.width() * zoomFactor)),
                     qMax(1, qRound(rotatedSize.height() * zoomFactor)));
    const qreal pixelRatio = devicePixelRatioF();
    const QSize renderSize(qRound(size.width() * pixelRatio), qRound(size.height() * pixelRatio));
    QPixmap displayedPixmap;
    if (m_isCurvedTrack && !m_curveArtworkTransform.isIdentity()) {
        displayedPixmap = QPixmap(renderSize);
        displayedPixmap.fill(Qt::transparent);
        QPainter painter(&displayedPixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        const QTransform rotation = QTransform().rotate(rotationDegrees());
        const QRectF bounds = rotation.mapRect(QRectF(QPointF(), m_unscaledSize));
        painter.scale(renderSize.width() / bounds.width(), renderSize.height() / bounds.height());
        painter.translate(-bounds.topLeft());
        painter.rotate(rotationDegrees());
        // Keep the photo's sleeper body inside the actual eight-stud track width.
        // Replace the terminal sleepers and couplings with the straight track's
        // matching end artwork; retain the calibrated curved body between them.
        const QPixmap straightEnds(QStringLiteral(":/images/StrightTrack.png"));
        const bool matchingEnds = straightEnds.size() == QSize(1142, 1377);
        const qreal startStation = matchingEnds ? 1.0 : -0.8;
        const qreal endStation = matchingEnds ? 15.0 : 16.8;
        const qreal pitch = m_unscaledFootprint.width() / 17.0;
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
        painter.drawPixmap(QPointF(), matchingCurveArtwork());
        painter.restore();
        if (matchingEnds) {
            // Use the same eight-stud width and 78 source pixels per lengthwise
            // stud as the calibrated straight piece. Couplings straddle the seam.
            const auto drawEnd = [&](const QPointF &center, qreal angle,
                                     const QRectF &source, qreal seamY) {
                painter.save();
                painter.translate(center * pitch);
                painter.rotate(angle);
                painter.drawPixmap(QRectF(-4 * pitch, (source.y() - seamY) / 78.0 * pitch,
                                          8 * pitch, source.height() / 78.0 * pitch),
                                   straightEnds, source);
                painter.restore();
            };
            drawEnd(QPointF(1, 4), 90.0, QRectF(310, 1209, 640, 135), 1294.0);
            const qreal sweep = qDegreesToRadians(LegoGrid::curveSweepDegrees);
            drawEnd(QPointF(1 + LegoGrid::curveRadiusStuds * std::sin(sweep),
                           4 + LegoGrid::curveRadiusStuds * (1 - std::cos(sweep))),
                    90.0 + LegoGrid::curveSweepDegrees,
                    QRectF(310, 0, 640, 132), 46.0);
        }
    } else {
        QPixmap source = m_originalPixmap.copy(m_sourceBounds);
        if (m_isStraightTrack) {
            // Mounting holes sit between the stud rows at the center of each
            // large sleeper. Render them before rotation so they follow the track.
            QPainter holes(&source);
            holes.setRenderHint(QPainter::Antialiasing);
            for (qreal y : {360.0, 670.0, 980.0}) {
                const QPointF center(source.width() / 2.0, y);
                QRadialGradient bevel(center, 23.0, center - QPointF(6, 6));
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
        const QPixmap artwork = source.transformed(
            QTransform().rotate(rotationDegrees()), Qt::SmoothTransformation);
        displayedPixmap = artwork.scaled(renderSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    displayedPixmap.setDevicePixelRatio(pixelRatio);
    setPixmap(displayedPixmap);
    setFixedSize(size);
}

QRectF CityLayoutElement::footprintRect() const
{
    const QTransform rotation = QTransform().rotate(rotationDegrees());
    const QRectF imageBounds = rotation.mapRect(QRectF(QPointF(), m_unscaledSize));
    QRectF footprint = rotation.mapRect(m_unscaledFootprint);
    footprint.translate(-imageBounds.topLeft());
    const qreal scaleX = width() / imageBounds.width();
    const qreal scaleY = height() / imageBounds.height();
    return QRectF(footprint.x() * scaleX, footprint.y() * scaleY,
                  footprint.width() * scaleX, footprint.height() * scaleY);
}

QPixmap CityLayoutElement::savedPixmap() const
{
    // Save full source detail in the same orientation as the displayed plate.
    return m_originalPixmap.transformed(
        QTransform().rotate(rotationDegrees()), Qt::SmoothTransformation);
}

void CityLayoutElement::rotateQuarterTurns(int turns)
{
    rotateByDegrees(turns * 90.0);
}

qreal CityLayoutElement::rotationStepDegrees() const
{
    return m_isCurvedTrack || m_plateSize == QSize(8, 16)
        ? LegoGrid::curveSweepDegrees : 90.0;
}

void CityLayoutElement::rotateSteps(int steps)
{
    rotateByDegrees(steps * rotationStepDegrees());
}

void CityLayoutElement::rotateByDegrees(qreal degrees)
{
    const QPointF center = QPointF(pos()) + footprintRect().center();
    m_rotationDegrees = std::fmod(m_rotationDegrees + degrees, 360.0);
    if (m_rotationDegrees < 0.0) m_rotationDegrees += 360.0;
    setZoomFactor(m_zoomFactor);
    const QPointF position = center - footprintRect().center();
    move(qRound(position.x()), qRound(position.y()));
}

QPointF CityLayoutElement::displayedPoint(const QPointF &point) const
{
    const QTransform rotation = QTransform().rotate(rotationDegrees());
    const QRectF bounds = rotation.mapRect(QRectF(QPointF(), m_unscaledSize));
    const QPointF rotated = rotation.map(point) - bounds.topLeft();
    return QPointF(rotated.x() * width() / bounds.width(),
                   rotated.y() * height() / bounds.height());
}

QList<QLineF> CityLayoutElement::trackConnections() const
{
    if (!m_isStraightTrack && !m_isCurvedTrack) return {};
    QList<QLineF> result;
    const qreal pitch = m_unscaledFootprint.width() / m_plateSize.width();
    const auto addConnection = [&](const QPointF &studPoint, qreal outwardDegrees) {
        const QPointF point = m_unscaledFootprint.topLeft() + studPoint * pitch;
        const qreal angle = qDegreesToRadians(outwardDegrees);
        const QPointF direction(std::cos(angle), std::sin(angle));
        result.append(QLineF(displayedPoint(point), displayedPoint(point + direction * pitch)));
    };
    if (m_isStraightTrack) {
        addConnection(QPointF(4, 0), -90);
        addConnection(QPointF(4, 16), 90);
    } else {
        const qreal angle = qDegreesToRadians(LegoGrid::curveSweepDegrees);
        addConnection(QPointF(0, 4), 180);
        addConnection(QPointF(LegoGrid::curveRadiusStuds * std::sin(angle),
                             4 + LegoGrid::curveRadiusStuds * (1 - std::cos(angle))),
                      LegoGrid::curveSweepDegrees);
    }
    return result;
}
