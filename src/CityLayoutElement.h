#pragma once

#include <QLabel>
#include <QPixmap>
#include <QString>
#include <QSizeF>

class CityLayoutElement final : public QLabel
{
public:
    explicit CityLayoutElement(const QString &name,
                               const QPixmap &pixmap,
                               const QSize &plateSize,
                               qreal pixelsPerStud,
                               qreal zoomFactor = 1.0,
                               QWidget *parent = nullptr);

    QString name() const;
    QSize plateSize() const { return m_plateSize; }
    void setZoomFactor(qreal zoomFactor);
    QPixmap savedPixmap() const;
    void rotateQuarterTurns(int turns);
    int rotationDegrees() const { return m_quarterTurns * 90; }

private:
    QString m_name;
    QPixmap m_originalPixmap;
    QSize m_plateSize;
    QSizeF m_unscaledSize;
    qreal m_zoomFactor = 1.0;
    int m_quarterTurns = 0;
};
