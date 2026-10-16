
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

#include <QJsonArray>
#include <QJsonDocument>

#include "CityLayoutView.h"
#include "LegoGrid.h"
#include "elements/CityLayoutElement.h"
#include "elements/ElementCatalog.h"
#include "LoadedProjects.h"

#include <QApplication>
#include <QColor>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QMenu>
#include <QInputDialog>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QWheelEvent>

#include <cmath>


namespace {
constexpr auto layoutElementMimeType = "application/x-legocity-layout-element";
constexpr auto layoutElementNameMimeType = "application/x-legocity-layout-element-name";
constexpr auto layoutElementSizeMimeType = "application/x-legocity-layout-element-size";
constexpr qreal minimumZoom = 0.25;
constexpr qreal maximumZoom = 24.0;
constexpr int snapDistance = 6;
constexpr qreal gridSpacing = LegoGrid::plateSpacing;
constexpr qreal pixelsPerStud = LegoGrid::pixelsPerStud;

bool isNonRotatableBasePlate(const CityLayoutElement *element)
{
    if (!element) return false;
    const QSize size = element->plateSize();
    if (size != QSize(32, 32) && size != QSize(48, 48)) return false;
    const auto *definition = ElementCatalog::instance().byResource(element->resourcePath());
    return definition && definition->tabName == QStringLiteral("Plates");
}

}


CityLayoutView::CityLayoutView(QWidget *parent)
    : QWidget(parent)
    , _Projects(LoadedProjects::instance())
{
    setMinimumWidth(150);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    m_zoomTimer.setSingleShot(false);
    m_zoomTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_zoomTimer, &QTimer::timeout, this, [this] {
        if (_Projects.currentProject() != m_zoomProject) {
            m_zoomTimer.stop();
            return;
        }
        const qreal zoom = m_zoomProject->zoomFactor;
        const qreal distance = std::log(m_requestedZoom / zoom);
        const bool finished = qAbs(distance) < 0.001;
        const qreal nextZoom = finished ? m_requestedZoom : zoom * std::exp(distance * 0.6);
        _Projects.setViewZoom(m_zoomAnchor, nextZoom);
        if (finished) m_zoomTimer.stop();
        update();
    });


}


void CityLayoutView::activateProject()
{
    m_zoomTimer.stop();
    m_zoomProject = nullptr;
    m_studLayer = QPixmap();
    m_studProject = nullptr;
    m_mouseDragged = false;
    m_isPanning = false;
    unsetCursor();
    if (_Projects.currentProject()) _Projects.cityLayouts().setDraggedElement(nullptr);
    update();
}


void CityLayoutView::dragEnterEvent(QDragEnterEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    if (event->mimeData()->hasFormat(layoutElementMimeType)) {
        event->acceptProposedAction();
    }
}


void CityLayoutView::dropEvent(QDropEvent *event)
{
    if (!_Projects.currentProject())
    {
        event->ignore();
        return;
    }

    const QString resourcePath = QString::fromUtf8(event->mimeData()->data(layoutElementMimeType));
    const QString name = QString::fromUtf8(event->mimeData()->data(layoutElementNameMimeType));
    const QJsonObject sizeData = QJsonDocument::fromJson(event->mimeData()->data(layoutElementSizeMimeType)).object();
    const QSize plateSize(sizeData.value(QStringLiteral("widthStuds")).toInt(),
                          sizeData.value(QStringLiteral("heightStuds")).toInt());
    const QPixmap pixmap(resourcePath);
    if (pixmap.isNull() || plateSize.width() <= 0 || plateSize.height() <= 0) {
        event->ignore();
        return;
    }

    auto *layoutElement = new CityLayoutElement(
        name, pixmap, plateSize, pixelsPerStud,
        _Projects.currentProject()->zoomFactor, this, resourcePath);

    const qreal initialRotation = sizeData.value(QStringLiteral("rotationDegrees")).toDouble();
    if (std::isfinite(initialRotation) && initialRotation != 0.0)
        layoutElement->rotateByDegrees(initialRotation);

    const QPoint requestedPosition = event->position().toPoint()
                                     - QPoint(layoutElement->width() / 2,
                                              layoutElement->height() / 2);
    layoutElement->setLayoutPosition(snappedPosition(requestedPosition, layoutElement));
    layoutElement->show();
    _Projects.cityLayouts().append(layoutElement);
    if (_Projects.currentProject()->trackRun.recordAdded(layoutElement, _Projects.cityLayouts())) {
        _Projects.cityLayouts().setSelectedElement(layoutElement);
        setFocus(Qt::OtherFocusReason);
    }
    ++_Projects.currentProject()->changeRevision;

    event->acceptProposedAction();
}


void CityLayoutView::mousePressEvent(QMouseEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPosition = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->buttons().testFlag(Qt::LeftButton))
    {
        const QPoint position = event->position().toPoint();
        m_mousePressPosition = position;
        m_mouseDragged = false;

        if ( !_Projects.cityLayouts().wasElementClicked(this,position) )
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
    if (!_Projects.currentProject()) { event->ignore(); return; }
    if (m_isPanning && event->buttons().testFlag(Qt::MiddleButton)) {
        const QPoint position = event->position().toPoint();
        panBy(position - m_lastPanPosition);
        m_lastPanPosition = position;
        event->accept();
        return;
    }

    Qt::MouseButtons button( event->buttons() );

    if ( button & Qt::LeftButton )
    {
        if ( _Projects.cityLayouts().dragElementOnMouseMove(this, event) )
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
                _Projects.cityLayouts().setSelectedElement(nullptr);
            }

            const QPoint offset = position - m_lastPanPosition;

            panBy(offset);

            m_lastPanPosition = position;
            event->accept();
            return;
        }
    }

    const QPoint position{ event->position().toPoint() };
    _Projects.cityLayouts().setSelectedElementAt(position);

    QWidget::mouseMoveEvent(event);
}


void CityLayoutView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        unsetCursor();
        event->accept();
        return;
    }

    Qt::MouseButtons button( event->button() );
    if (button == Qt::LeftButton )
    {
        if ( _Projects.cityLayouts().elementDragged( this, m_mouseDragged) )
        {
            if (m_mouseDragged) ++_Projects.currentProject()->changeRevision;
            m_mouseDragged = false;
            unsetCursor();
            event->accept();
            return;
        }

        if ( m_isPanning) {
            if (!m_mouseDragged) {
                _Projects.cityLayouts().setSelectedElement(nullptr);
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
    if (!_Projects.currentProject()) { event->ignore(); return; }
    const int wheelDelta = event->angleDelta().y() != 0
        ? event->angleDelta().y() : event->pixelDelta().y() * 2;
    if (wheelDelta == 0) {
        QWidget::wheelEvent(event);
        return;
    }
    auto *project = _Projects.currentProject();
    const qreal startingZoom = m_zoomTimer.isActive() && m_zoomProject == project
        ? m_requestedZoom : project->zoomFactor;
    m_requestedZoom = qBound(minimumZoom, startingZoom * std::pow(1.0015, wheelDelta), maximumZoom);
    m_zoomAnchor = event->position();
    m_zoomProject = project;
    // Accumulate wheel input and ease toward its target once per frame.
    if (!m_zoomTimer.isActive()) m_zoomTimer.start(16);
    event->accept();
}


void CityLayoutView::panBy(const QPoint &offset)
{
    for (CityLayoutElement *image : _Projects.cityLayouts()) {
        image->setLayoutPosition(image->layoutPosition() + offset);
    }
    _Projects.tableDefinition().translate(offset);
    _Projects.currentProject()->gridOrigin += offset;
    update();
}


bool CityLayoutView::undoDeletion()
{
    auto *project = _Projects.currentProject();
    if (!project || !_Projects.cityLayouts().undoDeletion(this, project->gridOrigin, project->zoomFactor))
        return false;
    ++project->changeRevision;
    setFocus(Qt::OtherFocusReason);
    update();
    return true;
}

void CityLayoutView::keyPressEvent(QKeyEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    if (event->matches(QKeySequence::Undo)) {
        undoDeletion();
        event->accept();
        return;
    }
    if (!(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))) {
        const bool add = event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal;
        const bool remove = event->key() == Qt::Key_Minus;
        auto *project = _Projects.currentProject();
        const bool changed = add
            ? project->trackRun.extend(_Projects.cityLayouts(), project->zoomFactor, this) != nullptr
            : remove && project->trackRun.trim(_Projects.cityLayouts());
        if (changed) {
            ++project->changeRevision;
            update();
            event->accept();
            return;
        }
    }
    if (event->key() == Qt::Key_R
        && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
        && _Projects.cityLayouts().selectedElement()) {
        rotateSelectedPlate(event->modifiers() & Qt::ShiftModifier ? -1 : 1);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace)
    {
        if ( _Projects.cityLayouts().deleteSelectedElement(
                _Projects.currentProject()->gridOrigin, _Projects.currentProject()->zoomFactor) )
        {
            ++_Projects.currentProject()->changeRevision;
            event->accept();
            return;
        }
    }

    QWidget::keyPressEvent(event);
}


void CityLayoutView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    // Clear exposed areas when child elements shrink or move during zoom/pan.
    // The grid and translucent table fill do not repaint the entire background.
    painter.fillRect(rect(), palette().brush(QPalette::Window));
    if (!_Projects.currentProject()) return;
    if (_Projects.tableEditor().gridVisible && !LoadedProjects::instance().title().isEmpty()) {
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(110, 110, 110, 210), 1));
        const qreal scaledGridSpacing = gridSpacing * _Projects.currentProject()->zoomFactor;
        qreal firstX = std::fmod(_Projects.currentProject()->gridOrigin.x(), scaledGridSpacing);
        qreal firstY = std::fmod(_Projects.currentProject()->gridOrigin.y(), scaledGridSpacing);
        if (firstX < 0.0) firstX += scaledGridSpacing;
        if (firstY < 0.0) firstY += scaledGridSpacing;
        for (qreal x = firstX; x <= width(); x += scaledGridSpacing) {
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
        }
        for (qreal y = firstY; y <= height(); y += scaledGridSpacing) {
            painter.drawLine(QPointF(0.0, y), QPointF(width(), y));
        }
    }

    painter.setRenderHint(QPainter::Antialiasing);
    if (!_Projects.tableDefinition().isEmpty()) {
        painter.setPen(QPen(QColor(80, 55, 30), 3));
        painter.setBrush(QColor(181, 143, 92, 120));
        const QPainterPath surface = _Projects.tableDefinition().usableArea();
        painter.drawPath(surface);
        if (_Projects.tableEditor().gridVisible) {
            const auto *project = _Projects.currentProject();
            const qreal zoom = project->zoomFactor;
            const QPointF origin = project->gridOrigin;
            const qreal dpr = devicePixelRatioF();
            const bool preview = m_zoomTimer.isActive() && !m_studLayer.isNull()
                && m_studProject == project && m_studZoom > 0.0;
            if (!preview) {
                // Include a margin so a typical zoom-out can reveal cached studs.
                const int margin = qMin(256, qMax(width(), height()) / 4);
                const QRect bounds = rect().adjusted(-margin, -margin, margin, margin);
                if (m_studLayer.isNull() || m_studLayerRect != bounds || m_studSurface != surface
                    || m_studOrigin != origin || !qFuzzyCompare(m_studZoom, zoom)
                    || m_studLayer.devicePixelRatioF() != dpr || m_studProject != project) {
                    m_studLayer = QPixmap(qRound(bounds.width() * dpr), qRound(bounds.height() * dpr));
                    m_studLayer.setDevicePixelRatio(dpr);
                    m_studLayer.fill(Qt::transparent);
                    QPainter studs(&m_studLayer);
                    studs.setRenderHint(QPainter::Antialiasing);
                    studs.translate(-bounds.topLeft());
                    LegoGrid::paintStuds(studs, surface, bounds, origin, zoom);
                    m_studLayerRect = bounds;
                    m_studSurface = surface;
                    m_studOrigin = origin;
                    m_studZoom = zoom;
                    m_studProject = _Projects.currentProject();
                }
            }
            if (LegoGrid::pixelsPerStud * zoom >= 4.0) {
                // Scale the existing layer for interactive frames; redraw precisely on settling.
                const qreal scale = zoom / m_studZoom;
                painter.save();
                painter.setClipPath(surface, Qt::IntersectClip);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);
                painter.translate(origin - m_studOrigin * scale);
                painter.scale(scale, scale);
                painter.drawPixmap(m_studLayerRect.topLeft(), m_studLayer);
                painter.restore();
            }
        }
    }
    if (!_Projects.tableDefinition().openSides().isEmpty()) {
        painter.setPen(QPen(QColor(80, 55, 30), 3));
        painter.setBrush(Qt::NoBrush);
        for (const QLineF &side : _Projects.tableDefinition().openSides()) {
            painter.drawLine(side);
        }
    }
}


void CityLayoutView::contextMenuEvent(QContextMenuEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    CityLayoutElement* layoutElement{ _Projects.cityLayouts().elementAt(event->pos()) };
    if (!layoutElement) {
        QMenu menu(this);
        QAction *undoAction = menu.addAction(tr("Undo Delete"));
        undoAction->setEnabled(_Projects.cityLayouts().canUndoDeletion());
        undoAction->setShortcut(QKeySequence::Undo);
        if (menu.exec(event->globalPos()) == undoAction) undoDeletion();
        event->accept();
        return;
    }

    setFocus(Qt::MouseFocusReason);
    _Projects.cityLayouts().setSelectedElement(layoutElement);

    QMenu menu(this);
    QAction *undoAction = menu.addAction(tr("Undo Delete"));
    undoAction->setEnabled(_Projects.cityLayouts().canUndoDeletion());
    undoAction->setShortcut(QKeySequence::Undo);
    menu.addSeparator();
    const bool isStraightTrack = layoutElement->isStraightTrack();
    qreal stepDegrees = layoutElement->rotationStepDegrees();
    const QString stepAngle = QString::number(stepDegrees);

    const auto *element = ElementCatalog::instance().byResource(layoutElement->resourcePath());
    const QSize plateSize = layoutElement->plateSize();

    const bool isRectangularBasePlate = element && element->tabName == QStringLiteral("Plates")
                                        && (plateSize == QSize(16,8) || plateSize == QSize(32,16));

    const bool isStraightRoadOrCrosswalk =
        layoutElement->resourcePath() == QStringLiteral(":/images/RoadPlate16x16.png")
        || layoutElement->resourcePath() == QStringLiteral(":/images/CrosswalkPlate8x16.png");

    const bool singleRotateOption = isRectangularBasePlate || isStraightRoadOrCrosswalk;
    const bool rotationAllowed = !isNonRotatableBasePlate(layoutElement);

    const bool customRotationOnly = layoutElement->resourcePath() == QStringLiteral(":/images/RoadIntersectionPlate16x16.png");

    const bool isRoadElement = isStraightRoadOrCrosswalk
        || layoutElement->resourcePath() == QStringLiteral(":/images/RoadTurnPlate16x16.png")
        || layoutElement->resourcePath() == QStringLiteral(":/images/RoadIntersectionPlate16x16.png");

    QAction *clockwiseAction = (!rotationAllowed || customRotationOnly) ? nullptr
        : menu.addAction(singleRotateOption
            ? tr("Rotate 90") : tr("Rotate Clockwise %1° (R)").arg(stepAngle));

    QAction *counterclockwiseAction = (!rotationAllowed || singleRotateOption || customRotationOnly) ? nullptr
        : menu.addAction(tr("Rotate Counterclockwise %1° (Shift+R)").arg(stepAngle));

    QAction *quarterClockwiseAction = nullptr;
    QAction *quarterCounterclockwiseAction = nullptr;
    if (rotationAllowed && !customRotationOnly && !singleRotateOption && !qFuzzyCompare(stepDegrees, 90.0))
    {   // if the stepDegrees is not 90, then we can add quarter rotation options
        if ( isStraightTrack )
        {
            quarterClockwiseAction = menu.addAction("Rotate 90°");
        }
        else
        {
            quarterClockwiseAction = menu.addAction("Rotate Clockwise 90°");
            quarterCounterclockwiseAction = menu.addAction("Rotate Counterclockwise 90°");
        }
    }

    QAction *customRotationAction = (rotationAllowed && isRoadElement)
        ? menu.addAction(tr("Custom Rotation...")) : nullptr;

    if (rotationAllowed) menu.addSeparator();

    QAction *bendAction = nullptr;
    QAction *straightenAction = nullptr;
    if (layoutElement->isFlexTrack()) {
        bendAction = menu.addAction(tr("Adjust Bend..."));
        straightenAction = menu.addAction(tr("Straighten Flex Track"));
        straightenAction->setEnabled(!qFuzzyIsNull(layoutElement->flexBendDegrees()));
        menu.addSeparator();
    }

    QAction *frontAction = menu.addAction(tr("Bring to Front"));
    QAction *backAction = menu.addAction(tr("Send to Back"));
    frontAction->setEnabled(_Projects.cityLayouts().last() != layoutElement);
    backAction->setEnabled(_Projects.cityLayouts().first() != layoutElement);

    menu.addSeparator();

    QAction *deleteAction = menu.addAction(tr("Delete"));
    QAction *chosenAction = menu.exec(event->globalPos());
    if (chosenAction == undoAction) {
        undoDeletion();
    } else if (clockwiseAction && chosenAction == clockwiseAction) {
        rotateSelectedPlate(1);
    } else if (counterclockwiseAction && chosenAction == counterclockwiseAction) {
        rotateSelectedPlate(-1);
    } else if (quarterClockwiseAction && chosenAction == quarterClockwiseAction) {
        rotateSelectedPlateByDegrees(90.0);
    } else if (quarterCounterclockwiseAction && chosenAction == quarterCounterclockwiseAction) {
        rotateSelectedPlateByDegrees(-90.0);
    } else if (customRotationAction && chosenAction == customRotationAction) {
        bool accepted = false;
        const qreal currentAngle = layoutElement->rotationDegrees();
        const qreal angle = QInputDialog::getDouble(this, tr("Custom Rotation"),
            tr("Rotation angle (° clockwise):"), currentAngle, 0.0, 360.0, 2, &accepted);
        const qreal difference = angle - currentAngle;
        if (accepted && !qFuzzyIsNull(std::remainder(difference, 360.0))) {
            rotateSelectedPlateByDegrees(difference);
        }
    } else if (bendAction && chosenAction == bendAction) {
        bool accepted = false;
        const qreal degrees = QInputDialog::getDouble(this, tr("Flex Track Bend"),
            tr("Bend angle (°): negative = left, positive = right"),
            layoutElement->flexBendDegrees(), -LegoGrid::flexMaximumBendDegrees,
            LegoGrid::flexMaximumBendDegrees, 3, &accepted);
        if (accepted && layoutElement->setFlexBendDegrees(degrees)) {
            ++_Projects.currentProject()->changeRevision;
            update();
        }
    } else if (straightenAction && chosenAction == straightenAction) {
        if (layoutElement->setFlexBendDegrees(0.0)) {
            ++_Projects.currentProject()->changeRevision;
            update();
        }
    } else if (chosenAction == frontAction) {
        if (_Projects.cityLayouts().bringSelectedToFront()) {
            ++_Projects.currentProject()->changeRevision;
        }
    } else if (chosenAction == backAction) {
        if (_Projects.cityLayouts().sendSelectedToBack()) {
            ++_Projects.currentProject()->changeRevision;
        }
    } else if (chosenAction == deleteAction) {
        if (_Projects.cityLayouts().deleteSelectedElement(
                _Projects.currentProject()->gridOrigin, _Projects.currentProject()->zoomFactor)) {
            ++_Projects.currentProject()->changeRevision;
        }
    }
    event->accept();
}


// void CityLayoutView::leaveEvent(QEvent *event)
// {
//     QWidget::leaveEvent(event);
// }


QPointF CityLayoutView::projectPoint(const QPointF &point) const
{
    return (point - _Projects.currentProject()->gridOrigin) / _Projects.currentProject()->zoomFactor;
}


QJsonObject CityLayoutView::savedLayout() const
{
    QJsonArray elements;
    for (const CityLayoutElement *element : _Projects.cityLayouts()) {
//        const auto *definition = ElementCatalog::instance().byResource(element->resourcePath());
        QJsonObject placement{
            {QStringLiteral("name"), element->name()},
            {QStringLiteral("x"), element->layoutPosition().x()},
            {QStringLiteral("y"), element->layoutPosition().y()},
            {QStringLiteral("rotationDegrees"), element->rotationDegrees()}};
        if (element->isFlexTrack()) placement.insert(QStringLiteral("flexBendDegrees"), element->flexBendDegrees());
        elements.append(placement);
    }
    return QJsonObject{{QStringLiteral("elements"), elements},
                       {QStringLiteral("zoomFactor"), _Projects.currentProject()->zoomFactor},
                       {QStringLiteral("gridOriginX"), _Projects.currentProject()->gridOrigin.x()},
                       {QStringLiteral("gridOriginY"), _Projects.currentProject()->gridOrigin.y()}};
}


quint64 CityLayoutView::changeRevision() const
{
    return _Projects.currentProject() ? _Projects.currentProject()->changeRevision : 0;
}


QPointF CityLayoutView::snappedPosition(const QPointF &position,
                                     const CityLayoutElement *movingElement) const
{
    if (!_Projects.currentProject()) return position;
    const auto *project = _Projects.currentProject();
    QPointF result = position;
    // Compatible rail ends take precedence over the underlying stud lattice.
    if (movingElement) {
        qreal nearest = snapDistance + 0.01;
        QPointF connectionOffset;
        bool foundConnection = false;
        for (const QLineF &moving : movingElement->trackConnections()) {
            if (qFuzzyIsNull(moving.length())) continue;
            const QPointF movingDirection = (moving.p2() - moving.p1()) / moving.length();
            for (const CityLayoutElement *other : _Projects.cityLayouts()) {
                if (other == movingElement) continue;
                for (const QLineF &target : other->trackConnections()) {
                    if (qFuzzyIsNull(target.length())) continue;
                    const QPointF targetDirection = (target.p2() - target.p1()) / target.length();
                    const qreal dot = QPointF::dotProduct(movingDirection, targetDirection);
                    if (dot > -0.996) continue;
                    const QPointF offset = other->layoutPosition() + target.p1()
                        - (QPointF(position) + moving.p1());
                    const qreal distance = QLineF(QPointF(), offset).length();
                    if (distance < nearest) {
                        nearest = distance;
                        connectionOffset = offset;
                        foundConnection = true;
                    }
                }
            }
        }
        if (foundConnection) return position + connectionOffset;
    }
    // A diagonal straight track or switch cannot align its body to the stud lattice.
    // Keep rail-end snapping above, then let unmatched diagonal tracks float.
    if (movingElement && (movingElement->isStraightTrack() || movingElement->isSwitchTrack() || movingElement->isFlexTrack())
        && qAbs(std::remainder(movingElement->rotationDegrees(), 90.0)) > 1e-6) {
        return position;
    }
    if (_Projects.tableEditor().snapToGrid) {
        // Align the sleeper/plate body, excluding image padding and connectors.
        const QPointF offset = movingElement ? movingElement->footprintRect().topLeft() : QPointF();
        return (LegoGrid::snappedPoint(QPointF(position) + offset,
                                      project->gridOrigin, project->zoomFactor) - offset);
    }
    if (!movingElement) return result;

    int closestX = snapDistance + 1;
    int closestY = snapDistance + 1;
    const QRectF movingBounds = movingElement->footprintRect().translated(position);
    const int left = qRound(movingBounds.left());
    const int top = qRound(movingBounds.top());
    const int right = qRound(movingBounds.right());
    const int bottom = qRound(movingBounds.bottom());
    for (const CityLayoutElement *other : _Projects.cityLayouts()) {
        if (other == movingElement) continue;
        const QRectF otherBounds = other->footprintRect().translated(other->layoutPosition());
        const int otherLeft = qRound(otherBounds.left());
        const int otherTop = qRound(otherBounds.top());
        const int otherRight = qRound(otherBounds.right());
        const int otherBottom = qRound(otherBounds.bottom());

        // Only attract nearby plates; distant edges sharing an axis should not snap.
        if (top <= otherBottom + snapDistance && bottom >= otherTop - snapDistance) {
            for (int movingEdge : {left, right}) {
                for (int targetEdge : {otherLeft, otherRight}) {
                    const int delta = targetEdge - movingEdge;
                    if (qAbs(delta) <= snapDistance && qAbs(delta) < closestX) {
                        closestX = qAbs(delta);
                        result.setX(position.x() + delta);
                    }
                }
            }
        }
        if (left <= otherRight + snapDistance && right >= otherLeft - snapDistance) {
            for (int movingEdge : {top, bottom}) {
                for (int targetEdge : {otherTop, otherBottom}) {
                    const int delta = targetEdge - movingEdge;
                    if (qAbs(delta) <= snapDistance && qAbs(delta) < closestY) {
                        closestY = qAbs(delta);
                        result.setY(position.y() + delta);
                    }
                }
            }
        }
    }
    return result;
}

void CityLayoutView::rotateSelectedPlate(int steps)
{
    if (!_Projects.currentProject()) return;
    CityLayoutElement *plate = _Projects.cityLayouts().selectedElement();
    if (!plate) return;
    rotateSelectedPlateByDegrees(steps * plate->rotationStepDegrees());
}

void CityLayoutView::rotateSelectedPlateByDegrees(qreal degrees)
{
    if (!_Projects.currentProject()) return;
    CityLayoutElement *plate = _Projects.cityLayouts().selectedElement();
    if (!plate || isNonRotatableBasePlate(plate)) return;
    plate->rotateByDegrees(degrees);
    if (_Projects.tableEditor().snapToGrid || !plate->trackConnections().isEmpty()) {
        plate->setLayoutPosition(snappedPosition(plate->layoutPosition(), plate));
    }
    ++_Projects.currentProject()->changeRevision;
    update();
}
