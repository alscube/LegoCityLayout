
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
#include <QJsonObject>
#include <QWidget>

class LoadedProjects;
class QDragEnterEvent;
class QDropEvent;
class QContextMenuEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class CityLayoutElement;


class CityLayoutView final : public QWidget
{
    Q_OBJECT

public:
    explicit CityLayoutView(QWidget *parent = nullptr);

    void activateProject();
    QPoint snappedPosition(const QPoint &position,
                           const CityLayoutElement *movingElement = nullptr) const;
    QPointF projectPoint(const QPointF &point) const;
    QJsonObject savedLayout() const;
    quint64 changeRevision() const;

    bool mouseDragged() const { return m_mouseDragged; }
    void setMouseDragged(bool dragged) { m_mouseDragged = dragged; }

    QPoint mousePressPosition() const { return m_mousePressPosition; }
    // void mousePressPosition(const QPoint &position) { m_mousePressPosition = position; }

    QPoint dragOffset() const { return m_dragOffset; }
    void setDragOffset(const QPoint &offset) { m_dragOffset = offset; }

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
//    void leaveEvent(QEvent *event) override;

private:
    void panBy(const QPoint &offset);
    void rotateSelectedPlate(int steps);
    void rotateSelectedPlateByDegrees(qreal degrees);

    LoadedProjects &_Projects;
    QPoint m_dragOffset;
    QPoint m_mousePressPosition;
    QPoint m_lastPanPosition;
    bool m_mouseDragged = false;
    bool m_isPanning = false;
};
