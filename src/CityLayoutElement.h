#pragma once

#include <QLabel>
#include <QPixmap>

class CityLayoutElement final : public QLabel
{
public:
    explicit CityLayoutElement(const QPixmap &pixmap,
                               qreal zoomFactor = 1.0,
                               QWidget *parent = nullptr);

    void setZoomFactor(qreal zoomFactor);

private:
    QPixmap m_originalPixmap;
};
