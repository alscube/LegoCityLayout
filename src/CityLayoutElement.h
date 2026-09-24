#pragma once

#include <QLabel>
#include <QPixmap>
#include <QString>

class CityLayoutElement final : public QLabel
{
public:
    explicit CityLayoutElement(const QString &name,
                               const QPixmap &pixmap,
                               qreal zoomFactor = 1.0,
                               QWidget *parent = nullptr);

    QString name() const;
    void setZoomFactor(qreal zoomFactor);

private:
    QString m_name;
    QPixmap m_originalPixmap;
};
