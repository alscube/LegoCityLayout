
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

    auto *layoutElement = new CityLayoutElement(pixmap, m_zoomFactor, this);

    const QPoint requestedPosition = event->position().toPoint()
                                     - QPoint(layoutElement->width() / 2,
                                              layoutElement->height() / 2);
    layoutElement->move(requestedPosition);
    layoutElement->show();
    _LayoutElements.append(layoutElement);

    event->acceptProposedAction();
}


void CityLayoutView::mousePressEvent(QMouseEvent *event)
{
    if (event->buttons().testFlag(Qt::LeftButton))
    {
        const QPoint position = event->position().toPoint();
        m_mousePressPosition = position;
        m_mouseDragged = false;

        if ( !_LayoutElements.wasElementClicked(this,position) )
        {
            m_isPanning = true;
            m_lastPanPosition = position;
        }

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
        if ( _LayoutElements.dragElementOnMouseMove(this, event) )
        {
            return;
        }

        if (m_isPanning )
        {
            const QPoint position = event->position().toPoint();
            if (!m_mouseDragged) {
                const int distance = (position - m_mousePressPosition).manhattanLength();
                if (distance < QApplication::startDragDistance()) {
                    event->accept();
                    return;
                }

                m_mouseDragged = true;
                _LayoutElements.setSelectedElement(nullptr);
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

    const QPoint position{ event->position().toPoint() };
    _LayoutElements.setSelectedElementAt(position);

    QWidget::mouseMoveEvent(event);
}


void CityLayoutView::mouseReleaseEvent(QMouseEvent *event)
{
    Qt::MouseButtons button( event->button() );
    if (button == Qt::LeftButton )
    {
        if ( _LayoutElements.elementDragged( this, m_mouseDragged) )
        {
            m_mouseDragged = false;
            unsetCursor();
            event->accept();
            return;
        }

        if ( m_isPanning) {
            if (!m_mouseDragged) {
                _LayoutElements.setSelectedElement(nullptr);
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

    _LayoutElements.zoomAllElements( anchor, relativeScale, newZoom );

    m_zoomFactor = newZoom;
    event->accept();
}


void CityLayoutView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete )
    {
        if ( _LayoutElements.deleteSelectedElement() )
        {
            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}


void CityLayoutView::contextMenuEvent(QContextMenuEvent *event)
{
    CityLayoutElement* layoutElement{ _LayoutElements.elementAt(event->pos()) };
    if (!layoutElement) {
        QWidget::contextMenuEvent(event);
        return;
    }

    setFocus(Qt::MouseFocusReason);
    _LayoutElements.setSelectedElement(layoutElement);

    QMenu menu(this);
    QAction *deleteAction = menu.addAction(tr("Delete"));
    if (menu.exec(event->globalPos()) == deleteAction) {
        _LayoutElements.deleteSelectedElement();
    }
    event->accept();
}

// void CityLayoutView::leaveEvent(QEvent *event)
// {
//     QWidget::leaveEvent(event);
// }


