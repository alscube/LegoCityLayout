#include "TableDefinition.h"

const QList<TableSurface> &TableDefinition::surfaces() const
{
    return m_surfaces;
}

bool TableDefinition::addSurface(const TableSurface &surface)
{
    if (!surface.isValid()) {
        return false;
    }

    m_surfaces.append(surface);
    return true;
}

bool TableDefinition::setSurface(qsizetype index, const TableSurface &surface)
{
    if (index < 0 || index >= m_surfaces.size() || !surface.isValid()) {
        return false;
    }

    m_surfaces[index] = surface;
    return true;
}

bool TableDefinition::removeSurface(qsizetype index)
{
    if (index < 0 || index >= m_surfaces.size()) {
        return false;
    }

    m_surfaces.removeAt(index);
    return true;
}

void TableDefinition::clear()
{
    m_surfaces.clear();
}

void TableDefinition::translate(const QPointF &offset)
{
    for (TableSurface &surface : m_surfaces) {
        QPolygonF outline = surface.outline();
        outline.translate(offset);
        surface.setOutline(outline);
    }
}

void TableDefinition::scale(const QPointF &anchor, qreal factor)
{
    for (TableSurface &surface : m_surfaces) {
        QPolygonF outline = surface.outline();
        for (QPointF &point : outline) {
            point = anchor + (point - anchor) * factor;
        }
        surface.setOutline(outline);
    }
}

QPainterPath TableDefinition::usableArea() const
{
    QPainterPath area;
    area.setFillRule(Qt::WindingFill);
    for (const TableSurface &surface : m_surfaces) {
        QPainterPath surfaceArea;
        surfaceArea.addPolygon(surface.outline());
        surfaceArea.closeSubpath();
        area = area.united(surfaceArea);
    }
    return area;
}

bool TableDefinition::contains(const QPointF &point) const
{
    return usableArea().contains(point);
}

bool TableDefinition::isEmpty() const
{
    return m_surfaces.isEmpty();
}
