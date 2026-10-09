
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

#include <QPoint>
#include <QString>
#include <QSize>
#include <QWidget>

class QMouseEvent;

// Studs
//       Each stud has a diameter of 4.8 mm
//       Height of roughly 1.7 mm
//       Space between studs 3.2 MM
//       Each stud measures 8 mm center to center

// Plates
//       Plate Height, equivalent to 1/3 of a standard LEGO plate (~0.32 cm / 0.1 in.)
//
// 8x16 - 128 studs
//      ~ 2.5x5 (or 2.6 in. x 5.1 in.)
//      ~ 6.4x12.8 (or 6.4 cm x 12.8 cm)
//
// 16x32 - 512 studs
//       ~ 5x10 (or 5.1 in. x 10.1 in.)
//       ~ 12x25 (or 12.8 cm x 25.6 cm)
//
// 32x32 - 1,024 studs
//       ~10x10 (or 10.1 in. x 10.1 in.),
//       ~25x25 (or 25.6 cm x 25.6 cm)
//
// 48x48 - 1,764 studs
//       ~13x13 (13.2 inches × 13.2 inches)
//       ~33x33 (33.6 cm × 33.6 cm)

// Track (stright)
//       Length: 16 studs (128 mm / ~5 inches)
//       Width: 8 studs (64 mm / ~2.5 inches across the plastic ties/sleepers)

// Track (Curved)
//       Length: 16 studs (128 mm / ~5 inches) - on center line
//       Radius: 40 studs (measured from the center of the track).
//       Angle:  22.5° per piece (16 pieces make a full 360° circle;
//               4 pieces make a 90° quarter-turn)


class LayoutElement final : public QWidget
{
public:
    LayoutElement(const QString &name, const QString &resourcePath,
                  const QSize &plateSize, QWidget *parent = nullptr);

    QSize plateSize() const { return m_plateSize; }

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QString m_name;
    QString m_resourcePath;
    QSize m_plateSize; // Width and height in studs.
    QPoint m_dragStartPosition;
};
