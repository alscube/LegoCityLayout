
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

    if (_SelectedElement == image) {
        _SelectedElement = nullptr;
    }

    if (_DraggedElement == image) {
        _DraggedElement = nullptr;
    }

    delete image;
}

