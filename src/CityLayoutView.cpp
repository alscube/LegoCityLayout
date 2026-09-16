
#include "CityLayoutView.h"
#include "CityLayoutElement.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QWheelEvent>

#include <cmath>

namespace {
constexpr auto layoutElementMimeType = "application/x-legocity-layout-element";
constexpr qreal minimumZoom = 0.25;
constexpr qreal maximumZoom = 4.0;
constexpr int snapDistance = 6;

}

CityLayoutView::CityLayoutView(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(150);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    auto *title = new QLabel(tr("City Layout"), this);
    title->setAttribute(Qt::WA_TransparentForMouseEvents);
    title->move(12, 12);
    title->show();
}

void CityLayoutView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(layoutElementMimeType)) {
        event->acceptProposedAction();
    }
}

void CityLayoutView::dropEvent(QDropEvent *event)
{
    const QString resourcePath = QString::fromUtf8(
        event->mimeData()->data(layoutElementMimeType));
    const QPixmap pixmap(resourcePath);
    if (pixmap.isNull()) {
        event->ignore();
        return;
    }

    auto *image = new CityLayoutElement(pixmap, m_zoomFactor, this);

    const QPoint requestedPosition = event->position().toPoint()
                                     - QPoint(image->width() / 2,
                                              image->height() / 2);
    image->move(requestedPosition);
    image->show();
    _LayoutElements.append(image);

    event->acceptProposedAction();
}

void CityLayoutView::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::LeftButton)) {
        const QPoint position = event->position().toPoint();
        m_mousePressPosition = position;
        m_mouseDragged = false;

        for (qsizetype index = _LayoutElements.size(); index-- > 0;) {
            CityLayoutElement *image = _LayoutElements.at(index);
            if (!image->geometry().contains(position)) {
                continue;
            }

            _DraggedElement = image;
            m_dragOffset = position - image->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }

        m_isPanning = true;
        m_lastPanPosition = position;
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void CityLayoutView::mouseMoveEvent(QMouseEvent *event)
{
    Qt::MouseButtons button( event->buttons() );

    if ( button & Qt::LeftButton )
    {
    if (_DraggedElement) {
        const QPoint position = event->position().toPoint();
        if (!m_mouseDragged) {
            const int distance = (position - m_mousePressPosition).manhattanLength();
            if (distance < QApplication::startDragDistance()) {
                event->accept();
                return;
            }

            m_mouseDragged = true;
            setFocus(Qt::MouseFocusReason);
            setSelectedImage(_DraggedElement);
            _LayoutElements.removeAll(_DraggedElement);
            _LayoutElements.append(_DraggedElement);
            _DraggedElement->raise();
        }

        const QPoint requestedPosition = position - m_dragOffset;
        const int x = requestedPosition.x();
        const int y = requestedPosition.y();
        int snappedX = x;
        int snappedY = y;
        int closestHorizontalSnap = snapDistance + 1;
        int closestVerticalSnap = snapDistance + 1;

        for (CityLayoutElement *image : _LayoutElements) {
            if (image == _DraggedElement) {
                continue;
            }

            const QRect draggedGeometry(x, y,
                                        _DraggedElement->width(),
                                        _DraggedElement->height());
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

        _DraggedElement->move(snappedX, snappedY);
        event->accept();
        return;
    }


    if (m_isPanning ) {
        const QPoint position = event->position().toPoint();
        if (!m_mouseDragged) {
            const int distance = (position - m_mousePressPosition).manhattanLength();
            if (distance < QApplication::startDragDistance()) {
                event->accept();
                return;
            }

            m_mouseDragged = true;
            setSelectedImage(nullptr);
        }

        const QPoint offset = position - m_lastPanPosition;

        for (CityLayoutElement *image : _LayoutElements) {
            image->move(image->pos() + offset);
        }

        m_lastPanPosition = position;
        event->accept();
        return;
    }
    }

    setHoveredImage(imageAt(event->position().toPoint()));

    QWidget::mouseMoveEvent(event);
}

void CityLayoutView::mouseReleaseEvent(QMouseEvent *event)
{
    Qt::MouseButtons button( event->button() );
    if (button == Qt::LeftButton )
    {
        if ( _DraggedElement) {
            if (!m_mouseDragged) {
                setFocus(Qt::MouseFocusReason);
                setSelectedImage(_DraggedElement);
                _LayoutElements.removeAll(_DraggedElement);
                _LayoutElements.append(_DraggedElement);
                _DraggedElement->raise();
            }

            _DraggedElement = nullptr;
            m_mouseDragged = false;
            unsetCursor();
            event->accept();
            return;
        }

        if ( m_isPanning) {
            if (!m_mouseDragged) {
                setSelectedImage(nullptr);
            }

            m_isPanning = false;
            m_mouseDragged = false;
            unsetCursor();
            event->accept();
            return;
        }
    }

    QWidget::mouseReleaseEvent(event);
}


void CityLayoutView::wheelEvent(QWheelEvent *event)
{
    const int wheelDelta = event->angleDelta().y();
    if (wheelDelta == 0) {
        QWidget::wheelEvent(event);
        return;
    }

    const qreal requestedZoom = m_zoomFactor * std::pow(1.0015, wheelDelta);
    const qreal newZoom = qBound(minimumZoom, requestedZoom, maximumZoom);
    if (qFuzzyCompare(newZoom, m_zoomFactor)) {
        event->accept();
        return;
    }

    const qreal relativeScale = newZoom / m_zoomFactor;
    const QPointF anchor = event->position();

    for (CityLayoutElement *image : _LayoutElements) {
        const QPointF position = anchor
                                 + (QPointF(image->pos()) - anchor) * relativeScale;
        image->setZoomFactor(newZoom);
        image->move(qRound(position.x()), qRound(position.y()));
    }

    m_zoomFactor = newZoom;
    event->accept();
}

void CityLayoutView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete ) {
        CityLayoutElement *image = _LayoutElements.selectedElement()
            ? _LayoutElements.selectedElement()
            : _LayoutElements.hoveredElement();
        if (image) {
            clearImageReferences(image);
            _LayoutElements.deleteImage(image);
            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}

void CityLayoutView::contextMenuEvent(QContextMenuEvent *event)
{
    CityLayoutElement *image = imageAt(event->pos());
    if (!image) {
        QWidget::contextMenuEvent(event);
        return;
    }

    setFocus(Qt::MouseFocusReason);
    setSelectedImage(image);
    setHoveredImage(image);

    QMenu menu(this);
    QAction *deleteAction = menu.addAction(tr("Delete"));
    if (menu.exec(event->globalPos()) == deleteAction) {
        clearImageReferences(image);
        _LayoutElements.deleteImage(image);
    }
    event->accept();
}

void CityLayoutView::leaveEvent(QEvent *event)
{
    setHoveredImage(nullptr);
    QWidget::leaveEvent(event);
}

CityLayoutElement *CityLayoutView::imageAt(const QPoint &position) const
{
    for (qsizetype index = _LayoutElements.size(); index-- > 0;) {
        CityLayoutElement *image = _LayoutElements.at(index);
        if (image->geometry().contains(position)) {
            return image;
        }
    }
    return nullptr;
}

void CityLayoutView::setHoveredImage( CityLayoutElement* newElement )
{
    CityLayoutElement* previousElement( _LayoutElements.hoveredElement() );
    if (previousElement == newElement ) {
        return;
    }

    _LayoutElements.setHoveredElement( newElement );
    updateElementHighLite( previousElement );
    updateElementHighLite( newElement );
}


void CityLayoutView::setSelectedImage(CityLayoutElement* newElement)
{
    CityLayoutElement* previousElement{ _LayoutElements.selectedElement() };
    if (previousElement == newElement) {
        return;
    }

    _LayoutElements.setSelectedElement(newElement);
    updateElementHighLite( previousElement );
    updateElementHighLite( newElement );
}


void CityLayoutView::updateElementHighLite(CityLayoutElement* element)
{
    if (!element) {
        return;
    }

    CityLayoutElement* hoveredElement( _LayoutElements.hoveredElement() );
    CityLayoutElement* selectedElement{ _LayoutElements.selectedElement() };
    const bool highlighted = (element == hoveredElement)
                             || (element == selectedElement);
    element->setStyleSheet(highlighted
        ? QStringLiteral("border: 2px solid orange;")
        : QString());
}


void CityLayoutView::clearImageReferences(CityLayoutElement *image)
{
    if (_DraggedElement == image) {
        _DraggedElement = nullptr;
    }
}
