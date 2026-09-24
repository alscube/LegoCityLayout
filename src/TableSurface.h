#pragma once

#include <QPolygonF>
#include <QString>

class TableSurface final
{
public:
    TableSurface() = default;
    TableSurface(const QString &name, const QPolygonF &outline);

    QString name() const;
    void setName(const QString &name);

    QPolygonF outline() const;
    bool setOutline(const QPolygonF &outline);

    bool isValid() const;
    bool contains(const QPointF &point) const;
    qreal area() const;

private:
    static bool isValidOutline(const QPolygonF &outline);

    QString m_name;
    QPolygonF m_outline;
};
