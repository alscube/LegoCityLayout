
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
