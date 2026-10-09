
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
