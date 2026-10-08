
#include "CityLayoutElement.h"

#include <QtMath>
#include <QTransform>

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
    QSizeF rotatedSize = m_unscaledSize;
    if (m_quarterTurns % 2) rotatedSize.transpose();
    const QSize size(qMax(1, qRound(rotatedSize.width() * zoomFactor)),
                     qMax(1, qRound(rotatedSize.height() * zoomFactor)));
    const qreal pixelRatio = devicePixelRatioF();
    const QPixmap artwork = m_originalPixmap.copy(m_sourceBounds).transformed(
        QTransform().rotate(rotationDegrees()), Qt::SmoothTransformation);
    QPixmap displayedPixmap = artwork.scaled(
        QSize(qRound(size.width() * pixelRatio), qRound(size.height() * pixelRatio)),
        Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
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
    const QPointF center = QPointF(pos()) + footprintRect().center();
    m_quarterTurns = ((m_quarterTurns + turns % 4) % 4 + 4) % 4;
    setZoomFactor(m_zoomFactor);
    const QPointF position = center - footprintRect().center();
    move(qRound(position.x()), qRound(position.y()));
}
