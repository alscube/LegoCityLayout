
#include "CityLayoutView.h"

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
#include <QVariant>
#include <QWheelEvent>

#include <cmath>

namespace {
constexpr auto layoutElementMimeType = "application/x-legocity-layout-element";
constexpr auto originalPixmapProperty = "originalPixmap";
constexpr qreal minimumZoom = 0.25;
constexpr qreal maximumZoom = 4.0;
constexpr int snapDistance = 6;

QPixmap scaledPixmap(const QPixmap &pixmap, qreal zoomFactor)
{
    const QSize size(qMax(1, qRound(pixmap.width() * zoomFactor)),
                     qMax(1, qRound(pixmap.height() * zoomFactor)));
    return pixmap.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}
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

    auto *image = new QLabel(this);
    const QPixmap displayedPixmap = scaledPixmap(pixmap, m_zoomFactor);
    image->setAttribute(Qt::WA_TransparentForMouseEvents);
    image->setProperty(originalPixmapProperty, QVariant::fromValue(pixmap));
    image->setPixmap(displayedPixmap);
    image->setFixedSize(displayedPixmap.size());

    const QPoint requestedPosition = event->position().toPoint()
                                     - QPoint(displayedPixmap.width() / 2,
                                              displayedPixmap.height() / 2);
    const int x = qBound(0, requestedPosition.x(),
                         qMax(0, width() - displayedPixmap.width()));
    const int y = qBound(0, requestedPosition.y(),
                         qMax(0, height() - displayedPixmap.height()));
    image->move(x, y);
    image->show();
    m_images.append(image);

    event->acceptProposedAction();
}

void CityLayoutView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        const QPoint position = event->position().toPoint();

        for (qsizetype index = m_images.size(); index-- > 0;) {
            QLabel *image = m_images.at(index);
            if (!image->geometry().contains(position)) {
                continue;
            }

            setFocus(Qt::MouseFocusReason);
            setSelectedImage(image);
            m_draggedImage = image;
            m_dragOffset = position - image->pos();
            m_images.removeAt(index);
            m_images.append(image);
            image->raise();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }

        setSelectedImage(nullptr);
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
    if (m_draggedImage && (event->buttons() & Qt::LeftButton)) {
        const QPoint requestedPosition = event->position().toPoint() - m_dragOffset;
        int x = qBound(0, requestedPosition.x(),
                       qMax(0, width() - m_draggedImage->width()));
        int y = qBound(0, requestedPosition.y(),
                       qMax(0, height() - m_draggedImage->height()));
        int snappedX = x;
        int snappedY = y;
        int closestHorizontalSnap = snapDistance + 1;
        int closestVerticalSnap = snapDistance + 1;

        for (QLabel *image : m_images) {
            if (image == m_draggedImage) {
                continue;
            }

            const QRect draggedGeometry(x, y,
                                        m_draggedImage->width(),
                                        m_draggedImage->height());
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

        x = qBound(0, snappedX, qMax(0, width() - m_draggedImage->width()));
        y = qBound(0, snappedY, qMax(0, height() - m_draggedImage->height()));
        m_draggedImage->move(x, y);
        event->accept();
        return;
    }

    if (m_isPanning && (event->buttons() & Qt::LeftButton)) {
        const QPoint position = event->position().toPoint();
        const QPoint offset = position - m_lastPanPosition;

        for (QLabel *image : m_images) {
            image->move(image->pos() + offset);
        }

        m_lastPanPosition = position;
        event->accept();
        return;
    }

    setHoveredImage(imageAt(event->position().toPoint()));

    QWidget::mouseMoveEvent(event);
}

void CityLayoutView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_draggedImage) {
        m_draggedImage = nullptr;
        unsetCursor();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_isPanning) {
        m_isPanning = false;
        unsetCursor();
        event->accept();
        return;
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

    for (QLabel *image : m_images) {
        const QPointF position = anchor
                                 + (QPointF(image->pos()) - anchor) * relativeScale;
        const QPixmap originalPixmap =
            image->property(originalPixmapProperty).value<QPixmap>();
        const QPixmap displayedPixmap = scaledPixmap(originalPixmap, newZoom);

        image->setPixmap(displayedPixmap);
        image->setFixedSize(displayedPixmap.size());
        image->move(qRound(position.x()), qRound(position.y()));
    }

    m_zoomFactor = newZoom;
    event->accept();
}

void CityLayoutView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        QLabel *image = m_selectedImage ? m_selectedImage : m_hoveredImage;
        if (image) {
            deleteImage(image);
            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}

void CityLayoutView::contextMenuEvent(QContextMenuEvent *event)
{
    QLabel *image = imageAt(event->pos());
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
        deleteImage(image);
    }
    event->accept();
}

void CityLayoutView::leaveEvent(QEvent *event)
{
    setHoveredImage(nullptr);
    QWidget::leaveEvent(event);
}

QLabel *CityLayoutView::imageAt(const QPoint &position) const
{
    for (qsizetype index = m_images.size(); index-- > 0;) {
        QLabel *image = m_images.at(index);
        if (image->geometry().contains(position)) {
            return image;
        }
    }
    return nullptr;
}

void CityLayoutView::setHoveredImage(QLabel *image)
{
    if (m_hoveredImage == image) {
        return;
    }

    QLabel *previousImage = m_hoveredImage;
    m_hoveredImage = image;
    updateImageHighlight(previousImage);
    updateImageHighlight(m_hoveredImage);
}

void CityLayoutView::setSelectedImage(QLabel *image)
{
    if (m_selectedImage == image) {
        return;
    }

    QLabel *previousImage = m_selectedImage;
    m_selectedImage = image;
    updateImageHighlight(previousImage);
    updateImageHighlight(m_selectedImage);
}

void CityLayoutView::updateImageHighlight(QLabel *image)
{
    if (!image) {
        return;
    }

    const bool highlighted = image == m_hoveredImage || image == m_selectedImage;
    image->setStyleSheet(highlighted
        ? QStringLiteral("border: 2px solid orange;")
        : QString());
}

void CityLayoutView::deleteImage(QLabel *image)
{
    if (!image) {
        return;
    }

    m_images.removeAll(image);
    if (m_draggedImage == image) {
        m_draggedImage = nullptr;
    }
    if (m_hoveredImage == image) {
        m_hoveredImage = nullptr;
    }
    if (m_selectedImage == image) {
        m_selectedImage = nullptr;
    }
    delete image;
}
