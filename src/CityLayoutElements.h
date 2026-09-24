#pragma once

#include <QList>


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

    CityLayoutElement* elementAt(const QPoint &position) const;

    CityLayoutElement *hoveredElement() const;
    void setHoveredElement( CityLayoutElement* newElement );

    CityLayoutElement* selectedElement() const;
    void setSelectedElement(CityLayoutElement* newElement);
    void setSelectedElementAt( const QPoint &position );

    void updateElementHighLite(CityLayoutElement* element);

    bool deleteSelectedElement( );

    bool wasElementClicked( CityLayoutView* view, const QPoint position );

    CityLayoutElement *draggedElement() const;
    void setDraggedElement(CityLayoutElement *element);

    bool dragElementOnMouseMove( CityLayoutView* view, QMouseEvent* event );

    bool elementDragged( QWidget *view, bool m_mouseDragged );

    void zoomAllElements( const QPointF anchor, const qreal relativeScale, qreal newZoom );

    void deleteImage(CityLayoutElement *image);

private:
    CityLayoutElement *_HoveredElement = nullptr;
    CityLayoutElement *_SelectedElement = nullptr;
    CityLayoutElement *_DraggedElement = nullptr;
};
