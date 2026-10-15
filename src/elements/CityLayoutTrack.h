
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

#include <QLineF>
#include <QList>
#include <QPixmap>
#include <QTransform>
#include <memory>

class CityLayoutElement;

// Track-specific artwork, calibration, connectors, bending, and selection.
// Non-track elements do not allocate this helper.
class CityLayoutTrack final
{
public:
    static std::unique_ptr<CityLayoutTrack> create(const QString &resourcePath);
    void calibrate(CityLayoutElement &element, qreal pixelsPerStud);
    QPixmap renderArtwork(const CityLayoutElement &element, const QSize &renderSize) const;
    void updateSelectionBounds(const QPixmap &displayedPixmap, const QSize &size);
    bool containsPoint(const QRect &rect, const QPoint &point) const;
    void paintSelection(CityLayoutElement &element) const;
    bool isStraightTrack() const { return m_isStraightTrack; }
    bool isSwitchTrack() const { return m_switchHandedness != 0; }
    bool isFlexTrack() const { return m_isFlexTrack; }
    qreal rotationStepDegrees() const;
    qreal flexBendDegrees() const { return m_flexBendDegrees; }
    bool setFlexBendDegrees(qreal degrees);
    QList<QLineF> connections(const CityLayoutElement &element) const;

private:
    explicit CityLayoutTrack(const QString &resourcePath);
    QPointF displayedPoint(const CityLayoutElement &element, const QPointF &point) const;
    QRect m_selectionBounds;
    int m_switchHandedness = 0; // -1 left, +1 right.
    bool m_isFlexTrack = false;
    qreal m_flexBendDegrees = 0.0;
    bool m_isCurvedTrack = false;
    bool m_isStraightTrack = false;
    QTransform m_curveArtworkTransform;
};
