
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

#include "TrackRun.h"
#include "CityLayoutElement.h"
#include "CityLayoutElements.h"
#include "LegoGrid.h"

#include <QtMath>
#include <memory>

namespace {
bool connected(const CityLayoutElement &first, const QLineF &firstPort,
               const CityLayoutElement &second, const QLineF &secondPort)
{
    if (qFuzzyIsNull(firstPort.length()) || qFuzzyIsNull(secondPort.length())) return false;
    const QPointF firstDirection = (firstPort.p2() - firstPort.p1()) / firstPort.length();
    const QPointF secondDirection = (secondPort.p2() - secondPort.p1()) / secondPort.length();
    return QPointF::dotProduct(firstDirection, secondDirection) < -0.996
        && QLineF(first.layoutPosition() + firstPort.p1(),
                  second.layoutPosition() + secondPort.p1()).length() <= 0.8;
}

bool occupied(const CityLayoutElement &element, const QLineF &port,
              const CityLayoutElements &layout)
{
    for (const auto *other : layout) {
        if (other == &element) continue;
        for (const auto &otherPort : other->trackConnections()) {
            if (connected(element, port, *other, otherPort)) return true;
        }
    }
    return false;
}
}

void TrackRun::prune(const CityLayoutElements &layout)
{
    for (qsizetype index = m_entries.size(); index-- > 0;) {
        if (!m_entries[index].element || !layout.contains(m_entries[index].element.data()))
            m_entries.removeAt(index);
    }
}

bool TrackRun::recordAdded(CityLayoutElement *element, const CityLayoutElements &layout)
{
    if (!element || !element->isTrack()) return false;
    prune(layout);
    const auto ports = element->trackConnections();
    if (ports.size() < 2) return false;
    int entrance = 0;
    bool continuesRun = false;
    if (!m_entries.isEmpty()) {
        const Entry &tail = m_entries.last();
        const auto tailPorts = tail.element->trackConnections();
        if (tail.exit < tailPorts.size()) {
            for (int port = 0; port < ports.size(); ++port) {
                if (connected(*element, ports[port], *tail.element, tailPorts[tail.exit])) {
                    entrance = port;
                    continuesRun = true;
                    break;
                }
            }
        }
    }
    if (!continuesRun) {
        m_entries.clear();
        // A new run may begin at the free end of an already connected piece.
        for (int port = 0; port < ports.size(); ++port) {
            if (occupied(*element, ports[port], layout)) {
                entrance = port;
                break;
            }
        }
    }
    m_entries.append(Entry{element, entrance, entrance == 1 ? 0 : 1});
    return true;
}

CityLayoutElement *TrackRun::extend(CityLayoutElements &layout, qreal zoom, QWidget *parent)
{
    prune(layout);
    if (m_entries.isEmpty()) recordAdded(layout.selectedElement(), layout);
    if (m_entries.isEmpty()) return nullptr;
    const Entry tail = m_entries.last();
    const auto *previous = tail.element.data();
    // Switches have multiple routes and are intentionally excluded from repetition.
    if (previous->isSwitchTrack()) return nullptr;
    const auto previousPorts = previous->trackConnections();
    if (tail.exit >= previousPorts.size() || occupied(*previous, previousPorts[tail.exit], layout))
        return nullptr;

    auto next = std::make_unique<CityLayoutElement>(previous->name(), previous->sourcePixmap(),
        previous->plateSize(), LegoGrid::pixelsPerStud, zoom, parent, previous->resourcePath());
    next->setFlexBendDegrees(previous->flexBendDegrees());
    next->rotateByDegrees(previous->rotationDegrees());
    auto nextPorts = next->trackConnections();
    if (tail.entrance >= nextPorts.size()) return nullptr;
    const auto &exit = previousPorts[tail.exit];
    const auto &entrance = nextPorts[tail.entrance];
    const QPointF exitDirection = exit.p2() - exit.p1();
    const QPointF entranceDirection = entrance.p2() - entrance.p1();
    const qreal turn = qRadiansToDegrees(std::atan2(exitDirection.y(), exitDirection.x())
        - std::atan2(entranceDirection.y(), entranceDirection.x())) + 180.0;
    next->rotateByDegrees(turn);
    nextPorts = next->trackConnections();
    const QPointF position = previous->layoutPosition() + exit.p1() - nextPorts[tail.entrance].p1();
    next->setLayoutPosition(position);
    next->show();
    auto *added = next.release();
    layout.append(added);
    m_entries.append(Entry{added, tail.entrance, tail.exit});
    layout.setSelectedElement(added);
    return added;
}

bool TrackRun::trim(CityLayoutElements &layout)
{
    prune(layout);
    if (m_entries.isEmpty()) return false;
    auto *removed = m_entries.takeLast().element.data();
    layout.deleteImage(removed);
    if (!m_entries.isEmpty()) layout.setSelectedElement(m_entries.last().element.data());
    return true;
}
