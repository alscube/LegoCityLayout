
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
#include <QString>
#include <QSizeF>
#include <memory>

class CityLayoutTrack;

class CityLayoutElement final : public QLabel
{
public:
    explicit CityLayoutElement(const QString &name,
                               const QPixmap &pixmap,
                               const QSize &plateSize,
                               qreal pixelsPerStud,
                               qreal zoomFactor = 1.0,
                               QWidget *parent = nullptr,
                               const QString &resourcePath = QString());

    ~CityLayoutElement() override;

    QString name() const;
    QString resourcePath() const { return m_resourcePath; }
    // Compatibility accessors delegate all track behavior to CityLayoutTrack.
    bool isStraightTrack() const;
    bool isSwitchTrack() const;
    bool isFlexTrack() const;
    bool isTrack() const { return bool(m_track); }
    qreal flexBendDegrees() const;
    bool setFlexBendDegrees(qreal degrees);
    bool containsPoint(const QPoint &localPoint) const;
    void setSelectionHighlighted(bool highlighted);
    QSize plateSize() const { return m_plateSize; }
    void setZoomFactor(qreal zoomFactor);
    void zoomFromAnchor(const QPointF &anchor, qreal relativeScale, qreal zoomFactor);
    // Logical canvas coordinates retain fractions between rounded QWidget moves.
    QPointF layoutPosition() const;
    void setLayoutPosition(const QPointF &position);
    QPixmap savedPixmap() const;
    // Stud-aligned body bounds, excluding artwork margins and connector overhangs.
    QRectF footprintRect() const;
    void rotateQuarterTurns(int turns);
    // Track-specific step sizes come from the optional helper; plates use quarter turns.
    qreal rotationStepDegrees() const;
    void rotateSteps(int steps);
    void rotateByDegrees(qreal degrees);
    qreal rotationDegrees() const { return m_rotationDegrees; }
    QPixmap sourcePixmap() const { return m_originalPixmap; }
    QList<QLineF> trackConnections() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    friend class CityLayoutTrack;
    std::unique_ptr<CityLayoutTrack> m_track;
    bool m_selectionHighlighted = false;
    QString m_name;
    QString m_resourcePath;
    QPixmap m_originalPixmap;
    QSize m_plateSize;
    QSizeF m_unscaledSize;
    QRect m_sourceBounds;
    QRectF m_unscaledFootprint;
    qreal m_zoomFactor = 1.0;
    qreal m_rotationDegrees = 0.0;
    QPointF m_precisePosition;
    QPoint m_lastZoomPosition;
    bool m_hasPrecisePosition = false;
};
