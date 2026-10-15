
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

#include <QList>
#include <QPointer>

class CityLayoutElement;
class CityLayoutElements;
class QWidget;

// Addition order for the active run, independent of the layout's drawing order.
class TrackRun final
{
public:
    bool recordAdded(CityLayoutElement *element, const CityLayoutElements &layout);
    CityLayoutElement *extend(CityLayoutElements &layout, qreal zoom, QWidget *parent);
    bool trim(CityLayoutElements &layout);

private:
    struct Entry {
        QPointer<CityLayoutElement> element;
        int entrance = 0;
        int exit = 1;
    };
    void prune(const CityLayoutElements &layout);
    QList<Entry> m_entries;
};
