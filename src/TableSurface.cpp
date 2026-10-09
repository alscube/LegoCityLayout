
// Copyright 2026. Alan Krzywicki

// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
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
