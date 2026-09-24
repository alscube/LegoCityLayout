
#include "CityLayoutElements.h"

#include "CityLayoutElement.h"
#include "CityLayoutView.h"

#include "QApplication"
#include "QMouseEvent"


constexpr int snapDistance = 6;



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


CityLayoutElement *CityLayoutElements::hoveredElement() const
{
    return _SelectedElement; // _HoveredElement;
}


void CityLayoutElements::setHoveredElement( CityLayoutElement* newElement )
{
    setSelectedElement( newElement );

    // CityLayoutElement* previousElement( _HoveredElement );
    // if (previousElement == newElement ) {
    //     return;
    // }

    // setSelectedElement( nullptr );

    // _HoveredElement = newElement;
    // updateElementHighLite( previousElement );
    // updateElementHighLite( newElement );
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

    if ( _SelectedElement )
       _SelectedElement->setStyleSheet(QString());

    const bool highlighted = (element == _HoveredElement || element == _SelectedElement);

    element->setStyleSheet(highlighted
                               ? QStringLiteral("border: 2px solid orange;")
                               : QString());
}


bool CityLayoutElements::deleteSelectedElement( )
{
    CityLayoutElement *image = ( selectedElement() ? selectedElement() : hoveredElement() );
    if (image) {
        deleteImage(image);
        return true;
    }

    return false;
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
    const int x = requestedPosition.x();
    const int y = requestedPosition.y();
    int snappedX = x;
    int snappedY = y;
    int closestHorizontalSnap = snapDistance + 1;
    int closestVerticalSnap = snapDistance + 1;

    for (CityLayoutElement* image : *this)
    {
        if (image == draggedElement) {
            continue;
        }

        const QRect draggedGeometry(x, y,
                                    draggedElement->width(),
                                    draggedElement->height());
        const QRect otherGeometry = image->geometry();

        const auto snapEdge = [](int draggedEdge, int otherEdge,
                                 int currentPosition, int &closestSnap,
                                 int &snappedPosition) {
            const int distance = qAbs(draggedEdge - otherEdge);
            if (distance <= snapDistance && distance < closestSnap) {
                closestSnap = distance;
                snappedPosition = currentPosition + otherEdge - draggedEdge;
            }
        };

        snapEdge(draggedGeometry.left(), otherGeometry.left(),
                 x, closestHorizontalSnap, snappedX);
        snapEdge(draggedGeometry.right(), otherGeometry.right(),
                 x, closestHorizontalSnap, snappedX);
        snapEdge(draggedGeometry.left(), otherGeometry.right() + 1,
                 x, closestHorizontalSnap, snappedX);
        snapEdge(draggedGeometry.right() + 1, otherGeometry.left(),
                 x, closestHorizontalSnap, snappedX);

        snapEdge(draggedGeometry.top(), otherGeometry.top(),
                 y, closestVerticalSnap, snappedY);
        snapEdge(draggedGeometry.bottom(), otherGeometry.bottom(),
                 y, closestVerticalSnap, snappedY);
        snapEdge(draggedGeometry.top(), otherGeometry.bottom() + 1,
                 y, closestVerticalSnap, snappedY);
        snapEdge(draggedGeometry.bottom() + 1, otherGeometry.top(),
                 y, closestVerticalSnap, snappedY);
    }

    draggedElement->move(snappedX, snappedY);

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
        removeAll(element);
        append(element);
        element->raise();
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
    if (_HoveredElement == image) {
        _HoveredElement = nullptr;
    }
    if (_SelectedElement == image) {
        _SelectedElement = nullptr;
    }
    if (_DraggedElement == image) {
        _DraggedElement = nullptr;
    }
    delete image;
}
