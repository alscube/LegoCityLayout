
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

#include "CityLayoutElements.h"

#include "CityLayoutElement.h"
#include "CityLayoutView.h"

#include "QApplication"
#include "QMouseEvent"




void CityLayoutElements::initializeCityElements(const QString &title)
{
    while (!isEmpty()) {
        delete takeLast();
    }

    _SelectedElement = nullptr;
    _DraggedElement = nullptr;
}


CityLayoutElement* CityLayoutElements::elementAt( const QPoint &position ) const
{
    for (qsizetype index = size(); index-- > 0;) {
        CityLayoutElement *image = at(index);
        if (image->geometry().contains(position)) {
            return image;
        }
    }
    return nullptr;
}


CityLayoutElement *CityLayoutElements::selectedElement() const
{
    return _SelectedElement;
}


void CityLayoutElements::setSelectedElement(CityLayoutElement* newElement)
{
    CityLayoutElement* previousElement = _SelectedElement;
    if (previousElement == newElement) {
        return;
    }

    _SelectedElement = newElement;
    updateElementHighLite( previousElement );
    updateElementHighLite( newElement );
}


    void CityLayoutElements::setSelectedElementAt( const QPoint &position )
{
    CityLayoutElement* newElement = elementAt(position);
    setSelectedElement(newElement);
}


void CityLayoutElements::updateElementHighLite(CityLayoutElement* element)
{
    if (!element) {
        return;
    }

    const bool highlighted = (element == _SelectedElement);

    element->setStyleSheet(highlighted
                               ? QStringLiteral("border: 2px solid orange;")
                               : QString());
}


bool CityLayoutElements::deleteSelectedElement( )
{
    CityLayoutElement *image = selectedElement();
    if (image) {
        deleteImage(image);
        return true;
    }

    return false;
}


bool CityLayoutElements::bringSelectedToFront()
{
    CityLayoutElement *element = selectedElement();
    if (!element || !contains(element) || last() == element) return false;
    removeAll(element);
    append(element);
    element->raise();
    return true;
}

bool CityLayoutElements::sendSelectedToBack()
{
    CityLayoutElement *element = selectedElement();
    if (!element || !contains(element) || first() == element) return false;
    removeAll(element);
    prepend(element);
    element->lower();
    return true;
}

CityLayoutElement *CityLayoutElements::draggedElement() const
{
    return _DraggedElement;
}

void CityLayoutElements::setDraggedElement(CityLayoutElement *element)
{
    _DraggedElement = element;
}


bool CityLayoutElements::wasElementClicked( CityLayoutView* view, const QPoint position )
{
    for (qsizetype index = size(); index-- > 0;)
    {
        CityLayoutElement *image = at(index);
        if (image->geometry().contains(position))
        {
            setDraggedElement(image);

            view->setDragOffset( position - image->pos() );

            return true;
        }
    }

    return false;
}

bool CityLayoutElements::dragElementOnMouseMove( CityLayoutView* view, QMouseEvent* event )
{
    CityLayoutElement *draggedElement = _DraggedElement;
    if ( !draggedElement ) {
        return false;
    }

    const QPoint position = event->position().toPoint();
    if (!view->mouseDragged()) {
        const int distance = (position - view->mousePressPosition()).manhattanLength();
        if (distance < QApplication::startDragDistance()) {
            event->accept();
            return true;
        }

        view->setMouseDragged( true );
        view->setFocus(Qt::MouseFocusReason);
        setSelectedElement(draggedElement);
        removeAll(draggedElement);
        append(draggedElement);
        draggedElement->raise();
    }

    const QPoint requestedPosition = position - view->dragOffset();
    draggedElement->move(view->snappedPosition(requestedPosition, draggedElement));

    event->accept();
    return true;
}


bool CityLayoutElements::elementDragged( QWidget *view, bool mouseDragged )
{
    CityLayoutElement* element{ draggedElement() };
    if ( element == nullptr )
        return false;

    if (!mouseDragged) {
        view->setFocus(Qt::MouseFocusReason);
        setSelectedElement(element);
    }

    setDraggedElement(nullptr);

    return true;
}


void CityLayoutElements::zoomAllElements( const QPointF anchor, const qreal relativeScale, qreal newZoom )
{
    for (CityLayoutElement *image : *this ) {//_LayoutElements) {
        const QPointF position = anchor
                                 + (QPointF(image->pos()) - anchor) * relativeScale;
        image->setZoomFactor(newZoom);
        image->move(qRound(position.x()), qRound(position.y()));
    }
}


void CityLayoutElements::deleteImage(CityLayoutElement *image)
{
    if (!image) {
        return;
    }

    removeAll(image);

    if (_SelectedElement == image) {
        _SelectedElement = nullptr;
    }

    if (_DraggedElement == image) {
        _DraggedElement = nullptr;
    }

    delete image;
}

