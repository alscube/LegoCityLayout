#pragma once

#include "TableSurface.h"

#include <QList>
#include <QLineF>
#include <QPainterPath>

class TableDefinition final
{
public:
    const QList<TableSurface> &surfaces() const;
    const QList<QLineF> &openSides() const;
    void setOpenSides(const QList<QLineF> &sides);

    bool addSurface(const TableSurface &surface);
    bool setSurface(qsizetype index, const TableSurface &surface);
    bool removeSurface(qsizetype index);
    void clear();
    void translate(const QPointF &offset);
    void scale(const QPointF &anchor, qreal factor);

    QPainterPath usableArea() const;
    bool contains(const QPointF &point) const;
    bool isEmpty() const;

private:
    QList<TableSurface> m_surfaces;
    QList<QLineF> m_openSides;
};
