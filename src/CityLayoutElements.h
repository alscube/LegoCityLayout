
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
#include <QString>


class CityLayoutElement;
class CityLayoutView;
class QWidget;
class QMouseEvent;
class QPoint;
class QPointF;


class CityLayoutElements final : public QList<CityLayoutElement *>
{
public:
    using QList<CityLayoutElement *>::QList;

    void initializeCityElements(const QString &title);

    CityLayoutElement* elementAt(const QPoint &position) const;

    CityLayoutElement* selectedElement() const;
    void setSelectedElement(CityLayoutElement* newElement);
    void setSelectedElementAt( const QPoint &position );

    void updateElementHighLite(CityLayoutElement* element);

    bool deleteSelectedElement( );
    bool bringSelectedToFront();
    bool sendSelectedToBack();

    bool wasElementClicked( CityLayoutView* view, const QPoint position );

    CityLayoutElement *draggedElement() const;
    void setDraggedElement(CityLayoutElement *element);

    bool dragElementOnMouseMove( CityLayoutView* view, QMouseEvent* event );

    bool elementDragged( QWidget *view, bool m_mouseDragged );

    void zoomAllElements( const QPointF anchor, const qreal relativeScale, qreal newZoom );

    void deleteImage(CityLayoutElement *image);

private:
    CityLayoutElement *_SelectedElement = nullptr;
    CityLayoutElement *_DraggedElement = nullptr;
};
