
#include "CityLayoutElement.h"

#include <QtMath>

CityLayoutElement::CityLayoutElement(const QString &name,
                                     const QPixmap &pixmap,
                                     qreal zoomFactor,
                                     QWidget *parent)
    : QLabel(parent), m_name(name), m_originalPixmap(pixmap)
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
    const QSize size(qMax(1, qRound(m_originalPixmap.width() * zoomFactor)),
                     qMax(1, qRound(m_originalPixmap.height() * zoomFactor)));
    const QPixmap displayedPixmap = m_originalPixmap.scaled(
        size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    setPixmap(displayedPixmap);
    setFixedSize(displayedPixmap.size());
}
