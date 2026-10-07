
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
    const QPixmap rotatedPixmap = m_originalPixmap.transformed(
        QTransform().rotate(rotationDegrees()), Qt::SmoothTransformation);
    const QPixmap displayedPixmap = rotatedPixmap.scaled(
        size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    setPixmap(displayedPixmap);
    setFixedSize(displayedPixmap.size());
}

void CityLayoutElement::rotateQuarterTurns(int turns)
{
    const QPointF center = QPointF(pos()) + QPointF(width() / 2.0, height() / 2.0);
    m_quarterTurns = ((m_quarterTurns + turns % 4) % 4 + 4) % 4;
    setZoomFactor(m_zoomFactor);
    move(qRound(center.x() - width() / 2.0),
         qRound(center.y() - height() / 2.0));
}
