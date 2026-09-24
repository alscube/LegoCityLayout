#include "TableSurface.h"

#include <QtMath>

namespace {
qreal signedArea(const QPolygonF &outline)
{
    qreal twiceArea = 0.0;
    for (qsizetype index = 0; index < outline.size(); ++index) {
        const QPointF &current = outline.at(index);
        const QPointF &next = outline.at((index + 1) % outline.size());
        twiceArea += current.x() * next.y() - next.x() * current.y();
    }
    return twiceArea / 2.0;
}
}

TableSurface::TableSurface(const QString &name, const QPolygonF &outline)
    : m_name(name)
{
    setOutline(outline);
}

QString TableSurface::name() const
{
    return m_name;
}

void TableSurface::setName(const QString &name)
{
    m_name = name;
}

QPolygonF TableSurface::outline() const
{
    return m_outline;
}

bool TableSurface::setOutline(const QPolygonF &outline)
{
    if (!isValidOutline(outline)) {
        return false;
    }

    m_outline = outline;
    if (m_outline.isClosed()) {
        m_outline.removeLast();
    }
    return true;
}

bool TableSurface::isValid() const
{
    return isValidOutline(m_outline);
}

bool TableSurface::contains(const QPointF &point) const
{
    return m_outline.containsPoint(point, Qt::OddEvenFill);
}

qreal TableSurface::area() const
{
    return qAbs(signedArea(m_outline));
}

bool TableSurface::isValidOutline(const QPolygonF &outline)
{
    QPolygonF normalized = outline;
    if (normalized.isClosed()) {
        normalized.removeLast();
    }

    return normalized.size() >= 3
           && !qFuzzyIsNull(signedArea(normalized));
}
