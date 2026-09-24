
#include "CityLayoutView.h"
#include "CityLayoutElement.h"

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
#include <QPushButton>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <cmath>


namespace {
constexpr auto layoutElementMimeType = "application/x-legocity-layout-element";
constexpr auto layoutElementNameMimeType = "application/x-legocity-layout-element-name";
constexpr qreal minimumZoom = 0.25;
constexpr qreal maximumZoom = 4.0;
constexpr int snapDistance = 6;
constexpr qreal pixelsPerInch = 12.8;
constexpr qreal gridSizeInches = 10.0;
constexpr qreal gridVisualScale = 0.5;
constexpr qreal gridSpacing = pixelsPerInch * gridSizeInches * gridVisualScale;
}


CityLayoutView::CityLayoutView(QWidget *parent)
    : QWidget(parent)
    , m_tableDefinitionEditor(m_tableDefinition)
{
    setMinimumWidth(150);
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    m_layoutPalette = palette();
    QPalette startupPalette = m_layoutPalette;
    startupPalette.setColor(QPalette::Window, QColor(52, 52, 52));
    setAutoFillBackground(true);
    setPalette(startupPalette);

    auto *viewLayout = new QVBoxLayout(this);
    viewLayout->addStretch();

    m_startupPanel = new QWidget(this);
    m_startupPanel->setMaximumWidth(280);
    auto *buttonLayout = new QVBoxLayout(m_startupPanel);
    buttonLayout->setSpacing(12);

    auto *createButton = new QPushButton(tr("Create New Layout"), m_startupPanel);
    auto *openButton = new QPushButton(tr("Open Layout"), m_startupPanel);
    auto *openLastButton = new QPushButton(tr("Open Last Layout"), m_startupPanel);
    buttonLayout->addWidget(createButton);
    buttonLayout->addWidget(openButton);
    buttonLayout->addWidget(openLastButton);

    connect(createButton, &QPushButton::clicked,
            this, &CityLayoutView::createNewLayoutRequested);
    connect(openButton, &QPushButton::clicked,
            this, &CityLayoutView::openLayoutRequested);
    connect(openLastButton, &QPushButton::clicked,
            this, &CityLayoutView::openLastLayoutRequested);

    viewLayout->addWidget(m_startupPanel, 0, Qt::AlignHCenter);
    viewLayout->addStretch();
}


void CityLayoutView::startProject(const QString &title)
{
    _LayoutElements.startProject(title);
    m_tableDefinition.clear();
    m_tableDefinitionEditor.reset(this);
    m_tableDefinitionEditor.resetViewScale();
    m_gridOrigin = QPointF();
    m_zoomFactor = 1.0;
    setPalette(m_layoutPalette);
    if (m_titleLabel) {
        m_titleLabel->setText(title);
        m_titleLabel->adjustSize();
    }
    m_startupPanel->hide();
    update();
}


QString CityLayoutView::projectTitle() const
{
    return _LayoutElements.projectTitle();
}


// TableDefinition &CityLayoutView::tableDefinition()
// {
//     return m_tableDefinition;
// }


// const TableDefinition &CityLayoutView::tableDefinition() const
// {
//     return m_tableDefinition;
// }


void CityLayoutView::beginTableDefinition()
{
    m_tableDefinitionEditor.begin(this);
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
    const QString name = QString::fromUtf8(
        event->mimeData()->data(layoutElementNameMimeType));
    const QPixmap pixmap(resourcePath);
    if (pixmap.isNull()) {
        event->ignore();
        return;
    }

    auto *layoutElement = new CityLayoutElement(name, pixmap, m_zoomFactor, this);

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
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_lastPanPosition = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (m_tableDefinitionEditor.mousePressEvent(this, event)) {
        return;
    }

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
    if (m_isPanning && event->buttons().testFlag(Qt::MiddleButton)) {
        const QPoint position = event->position().toPoint();
        panBy(position - m_lastPanPosition);
        m_lastPanPosition = position;
        event->accept();
        return;
    }

    if (m_tableDefinitionEditor.mouseMoveEvent(this, event)) {
        return;
    }

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

            panBy(offset);

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
    if (event->button() == Qt::MiddleButton && m_isPanning) {
        m_isPanning = false;
        unsetCursor();
        event->accept();
        return;
    }

    if (m_tableDefinitionEditor.mouseReleaseEvent(this, event)) {
        return;
    }

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
    m_tableDefinition.scale(anchor, relativeScale);
    m_tableDefinitionEditor.scale(anchor, relativeScale);
    m_gridOrigin = anchor + (m_gridOrigin - anchor) * relativeScale;

    m_zoomFactor = newZoom;
    update();
    event->accept();
}


void CityLayoutView::panBy(const QPoint &offset)
{
    for (CityLayoutElement *image : _LayoutElements) {
        image->move(image->pos() + offset);
    }
    m_tableDefinition.translate(offset);
    m_tableDefinitionEditor.translate(offset);
    m_gridOrigin += offset;
    update();
}


void CityLayoutView::keyPressEvent(QKeyEvent *event)
{
    if (m_tableDefinitionEditor.keyPressEvent(this, event)) {
        return;
    }

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


void CityLayoutView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    if (!projectTitle().isEmpty()) {
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(205, 205, 205, 150), 1));
        const qreal scaledGridSpacing = gridSpacing * m_zoomFactor;
        qreal firstX = std::fmod(m_gridOrigin.x(), scaledGridSpacing);
        qreal firstY = std::fmod(m_gridOrigin.y(), scaledGridSpacing);
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

    m_tableDefinitionEditor.paint(painter);
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
