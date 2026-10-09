
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

#include <QLabel>
#include <QLineF>
#include <QList>
#include <QPixmap>
#include <QTransform>
#include <QString>
#include <QSizeF>

class CityLayoutElement final : public QLabel
{
public:
    explicit CityLayoutElement(const QString &name,
                               const QPixmap &pixmap,
                               const QSize &plateSize,
                               qreal pixelsPerStud,
                               qreal zoomFactor = 1.0,
                               QWidget *parent = nullptr);

    QString name() const;
    QSize plateSize() const { return m_plateSize; }
    void setZoomFactor(qreal zoomFactor);
    QPixmap savedPixmap() const;
    // Stud-aligned body bounds, excluding artwork margins and connector overhangs.
    QRectF footprintRect() const;
    void rotateQuarterTurns(int turns);
    // Tracks turn in 22.5-degree steps; other elements use quarter turns.
    qreal rotationStepDegrees() const;
    void rotateSteps(int steps);
    void rotateByDegrees(qreal degrees);
    qreal rotationDegrees() const { return m_rotationDegrees; }
    QPixmap sourcePixmap() const { return m_originalPixmap; }
    QList<QLineF> trackConnections() const;

private:
    QString m_name;
    QPixmap m_originalPixmap;
    QSize m_plateSize;
    QSizeF m_unscaledSize;
    QRect m_sourceBounds;
    QRectF m_unscaledFootprint;
    qreal m_zoomFactor = 1.0;
    qreal m_rotationDegrees = 0.0;
    bool m_isCurvedTrack = false;
    bool m_isStraightTrack = false;
    QTransform m_curveArtworkTransform;
    QPointF displayedPoint(const QPointF &point) const;
};
