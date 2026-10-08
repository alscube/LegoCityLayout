
#include <QBuffer>
#include <QJsonArray>
#include <QJsonDocument>

#include "CityLayoutView.h"
#include "CityLayoutElement.h"
#include "LoadedProjects.h"

#include <QApplication>
#include <QColor>
#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QWheelEvent>
#include <QVBoxLayout>

#include <cmath>


namespace {
constexpr auto layoutElementMimeType = "application/x-legocity-layout-element";
constexpr auto layoutElementNameMimeType = "application/x-legocity-layout-element-name";
constexpr auto layoutElementSizeMimeType = "application/x-legocity-layout-element-size";
constexpr qreal minimumZoom = 0.25;
constexpr qreal maximumZoom = 24.0;
constexpr int snapDistance = 6;
constexpr qreal pixelsPerInch = 12.8;
constexpr qreal gridSizeInches = 10.0;
constexpr qreal gridVisualScale = 0.5;
constexpr qreal gridSpacing = pixelsPerInch * gridSizeInches * gridVisualScale;
// One city grid square represents a standard 32-stud baseplate.
constexpr qreal pixelsPerStud = gridSpacing / 32.0;
}


CityLayoutView::CityLayoutView(QWidget *parent)
    : QWidget(parent)
    , _Projects(LoadedProjects::instance())
{
    setMinimumWidth(150);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *titleLabel = new QLabel(tr("City Layout"), this);
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setContentsMargins(14, 10, 14, 10);
    titleLabel->setStyleSheet(
        "QLabel { background-color: palette(window); "
        "font-size: 22px; font-weight: 700; }");
    layout->addWidget(titleLabel);
    layout->addStretch();
}


void CityLayoutView::activateProject()
{
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
    if (!_Projects.currentProject()) { event->ignore(); return; }
    const QString resourcePath = QString::fromUtf8(
        event->mimeData()->data(layoutElementMimeType));
    const QString name = QString::fromUtf8(
        event->mimeData()->data(layoutElementNameMimeType));
    const QJsonObject sizeData = QJsonDocument::fromJson(
        event->mimeData()->data(layoutElementSizeMimeType)).object();
    const QSize plateSize(sizeData.value(QStringLiteral("widthStuds")).toInt(),
                          sizeData.value(QStringLiteral("heightStuds")).toInt());
    const QPixmap pixmap(resourcePath);
    if (pixmap.isNull() || plateSize.width() <= 0 || plateSize.height() <= 0) {
        event->ignore();
        return;
    }

    auto *layoutElement = new CityLayoutElement(
        name, pixmap, plateSize, pixelsPerStud,
        _Projects.currentProject()->zoomFactor, this);

    const QPoint requestedPosition = event->position().toPoint()
                                     - QPoint(layoutElement->width() / 2,
                                              layoutElement->height() / 2);
    layoutElement->move(snappedPosition(requestedPosition, layoutElement));
    layoutElement->show();
    _Projects.cityLayouts().append(layoutElement);
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
            ++_Projects.currentProject()->changeRevision;
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
    const int wheelDelta = event->angleDelta().y();
    if (wheelDelta == 0) {
        QWidget::wheelEvent(event);
        return;
    }

    const qreal requestedZoom = _Projects.currentProject()->zoomFactor * std::pow(1.0015, wheelDelta);
    const qreal newZoom = qBound(minimumZoom, requestedZoom, maximumZoom);
    if (qFuzzyCompare(newZoom, _Projects.currentProject()->zoomFactor)) {
        event->accept();
        return;
    }

    _Projects.setViewZoom(event->position(), newZoom);
    update();
    event->accept();
}


void CityLayoutView::panBy(const QPoint &offset)
{
    for (CityLayoutElement *image : _Projects.cityLayouts()) {
        image->move(image->pos() + offset);
    }
    _Projects.tableDefinition().translate(offset);
    _Projects.currentProject()->gridOrigin += offset;
    update();
}


void CityLayoutView::keyPressEvent(QKeyEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    if (event->key() == Qt::Key_R
        && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier))
        && _Projects.cityLayouts().selectedElement()) {
        rotateSelectedPlate(event->modifiers() & Qt::ShiftModifier ? -1 : 1);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete )
    {
        if ( _Projects.cityLayouts().deleteSelectedElement() )
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

    if (!_Projects.currentProject()) return;
    QPainter painter(this);
    if (_Projects.tableEditor().gridVisible && !LoadedProjects::instance().title().isEmpty()) {
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(205, 205, 205, 150), 1));
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
        painter.drawPath(_Projects.tableDefinition().usableArea());
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
        QWidget::contextMenuEvent(event);
        return;
    }

    setFocus(Qt::MouseFocusReason);
    _Projects.cityLayouts().setSelectedElement(layoutElement);

    QMenu menu(this);
    QAction *clockwiseAction = menu.addAction(tr("Rotate Clockwise (R)"));
    QAction *counterclockwiseAction = menu.addAction(tr("Rotate Counterclockwise (Shift+R)"));
    menu.addSeparator();
    QAction *deleteAction = menu.addAction(tr("Delete"));
    QAction *chosenAction = menu.exec(event->globalPos());
    if (chosenAction == clockwiseAction) {
        rotateSelectedPlate(1);
    } else if (chosenAction == counterclockwiseAction) {
        rotateSelectedPlate(-1);
    } else if (chosenAction == deleteAction) {
        if (_Projects.cityLayouts().deleteSelectedElement()) {
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
        QByteArray image;
        QBuffer buffer(&image);
        buffer.open(QIODevice::WriteOnly);
        element->savedPixmap().save(&buffer, "PNG");
        elements.append(QJsonObject{
            {QStringLiteral("name"), element->name()},
            {QStringLiteral("widthStuds"), element->plateSize().width()},
            {QStringLiteral("heightStuds"), element->plateSize().height()},
            {QStringLiteral("rotationDegrees"), element->rotationDegrees()},
            {QStringLiteral("x"), element->x()},
            {QStringLiteral("y"), element->y()},
            {QStringLiteral("imagePngBase64"), QString::fromLatin1(image.toBase64())}});
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


QPoint CityLayoutView::snappedPosition(const QPoint &position,
                                     const CityLayoutElement *movingElement) const
{
    if (!_Projects.currentProject()) return position;
    const auto *project = _Projects.currentProject();
    QPoint result = position;
    if (_Projects.tableEditor().snapToGrid) {
        const qreal spacing = gridSpacing * project->zoomFactor;
        const QPointF relative = position - project->gridOrigin;
        result = (project->gridOrigin
                  + QPointF(qRound(relative.x() / spacing) * spacing,
                            qRound(relative.y() / spacing) * spacing)).toPoint();
    }
    if (!movingElement) return result;

    int closestX = snapDistance + 1;
    int closestY = snapDistance + 1;
    const int left = position.x();
    const int top = position.y();
    const int right = left + movingElement->width();
    const int bottom = top + movingElement->height();
    for (const CityLayoutElement *other : _Projects.cityLayouts()) {
        if (other == movingElement) continue;
        const int otherLeft = other->x();
        const int otherTop = other->y();
        const int otherRight = otherLeft + other->width();
        const int otherBottom = otherTop + other->height();

        // Only attract nearby plates; distant edges sharing an axis should not snap.
        if (top <= otherBottom + snapDistance && bottom >= otherTop - snapDistance) {
            for (int movingEdge : {left, right}) {
                for (int targetEdge : {otherLeft, otherRight}) {
                    const int delta = targetEdge - movingEdge;
                    if (qAbs(delta) <= snapDistance && qAbs(delta) < closestX) {
                        closestX = qAbs(delta);
                        result.setX(left + delta);
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
                        result.setY(top + delta);
                    }
                }
            }
        }
    }
    return result;
}

void CityLayoutView::rotateSelectedPlate(int quarterTurns)
{
    if (!_Projects.currentProject()) return;
    CityLayoutElement *plate = _Projects.cityLayouts().selectedElement();
    if (!plate) return;
    plate->rotateQuarterTurns(quarterTurns);
    ++_Projects.currentProject()->changeRevision;
    update();
}
