#include "TableDefinitionEditorView.h"

#include "TableDefinition.h"
#include "TableSurface.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QWidget>
#include <QVBoxLayout>
#include <QtMath>

#include <cmath>

namespace {
constexpr qreal pixelsPerInch = 12.8;
constexpr qreal gridSizeInches = 10.0;
constexpr qreal gridVisualScale = 0.5;
constexpr qreal gridSpacing = pixelsPerInch * gridSizeInches * gridVisualScale;
constexpr qreal closePointDistance = 14.0;
constexpr qreal sideSelectionDistance = 8.0;

qreal distanceToSegment(const QPointF &point, const QLineF &segment)
{
    const qreal lengthSquared = segment.dx() * segment.dx()
                                + segment.dy() * segment.dy();
    if (qFuzzyIsNull(lengthSquared)) {
        return QLineF(point, segment.p1()).length();
    }

    const QPointF offset = point - segment.p1();
    const qreal projection = qBound(
        0.0,
        (offset.x() * segment.dx() + offset.y() * segment.dy()) / lengthSquared,
        1.0);
    const QPointF closest = segment.p1()
                            + QPointF(projection * segment.dx(),
                                      projection * segment.dy());
    return QLineF(point, closest).length();
}

bool getSideMeasurements(QWidget *parent, qreal suggestedLength,
                         qreal suggestedAngle,
                         qreal &length, qreal &angle)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QObject::tr("Side Measurement"));

    auto *layout = new QFormLayout(&dialog);
    auto *lengthInput = new QDoubleSpinBox(&dialog);
    lengthInput->setRange(0.1, 1000.0);
    lengthInput->setDecimals(2);
    lengthInput->setSuffix(QObject::tr(" in"));
    lengthInput->setValue(suggestedLength);

    auto *angleInput = new QDoubleSpinBox(&dialog);
    angleInput->setRange(0.0, 359.0);
    angleInput->setDecimals(1);
    angleInput->setSuffix(QObject::tr("°"));
    angleInput->setValue(suggestedAngle);
    angleInput->setToolTip(QObject::tr(
        "Screen-relative direction: 0° right, 90° down, 180° left, 270° up."));

    layout->addRow(QObject::tr("Side length:"), lengthInput);
    layout->addRow(QObject::tr("Screen angle:"), angleInput);

    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    QObject::connect(buttons, &QDialogButtonBox::accepted,
                     &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected,
                     &dialog, &QDialog::reject);
    layout->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    length = lengthInput->value();
    angle = angleInput->value();
    return true;
}
}

TableDefinitionEditorView::TableDefinitionEditorView(TableDefinition &definition,
                                             QWidget *parent)
    : QWidget(parent)
    , m_definition(definition)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setAutoFillBackground(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(0, 0, 10, 0);
    headerLayout->setSpacing(8);

    auto *titleLabel = new QLabel(tr("Table Definition"), header);
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setContentsMargins(14, 10, 14, 10);
    titleLabel->setStyleSheet(
        "QLabel { background-color: palette(window); "
        "font-size: 22px; font-weight: 700; }");
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();

    auto *gridButton = new QPushButton(tr("Hide Grid"), header);
    gridButton->setCheckable(true);
    gridButton->setChecked(true);
    gridButton->setToolTip(tr("Show or hide the 10-inch grid"));
    headerLayout->addWidget(gridButton);

    auto *snapButton = new QPushButton(tr("Snap to Grid"), header);
    snapButton->setCheckable(true);
    snapButton->setChecked(true);
    snapButton->setToolTip(tr("Snap drawn endpoints to grid intersections"));
    headerLayout->addWidget(snapButton);

    connect(gridButton, &QPushButton::toggled,
            this, [this, gridButton](bool checked) {
        m_gridVisible = checked;
        gridButton->setText(checked ? tr("Hide Grid") : tr("Show Grid"));
        update();
    });
    connect(snapButton, &QPushButton::toggled, this, [this](bool checked) {
        m_snapToGrid = checked;
        if (m_drawingSide) {
            m_cursor = snappedPoint(m_cursor);
        }
        update();
    });

    layout->addWidget(header);
    layout->addStretch();

    auto *instructions = new QLabel(
        tr("Press and drag from an endpoint to draw a side. Release on the "
           "starting point to close; Esc cancels and Delete undoes."), this);
    instructions->setAlignment(Qt::AlignCenter);
    instructions->setContentsMargins(12, 10, 12, 10);
    instructions->setStyleSheet(
        "QLabel { background-color: palette(window); "
        "border-top: 1px solid palette(mid); font-weight: 600; }");
    layout->addWidget(instructions);
}

void TableDefinitionEditorView::begin()
{
    m_draft.clear();
    m_selectedSurface = -1;
    m_selectedSide = -1;
    m_active = true;
    m_drawingSide = false;
    setCursor(Qt::CrossCursor);
    setFocus(Qt::OtherFocusReason);
    update();
}

void TableDefinitionEditorView::reset()
{
    m_draft.clear();
    m_active = false;
    m_drawingSide = false;
    m_selectedSurface = -1;
    m_selectedSide = -1;
    unsetCursor();
    update();
}

void TableDefinitionEditorView::resetViewScale()
{
    m_viewScale = 1.0;
}

QPointF TableDefinitionEditorView::snappedPoint(const QPointF &point) const
{
    if (!m_snapToGrid) {
        return point;
    }

    const qreal spacing = gridSpacing * m_viewScale;
    return QPointF(qRound(point.x() / spacing) * spacing,
                   qRound(point.y() / spacing) * spacing);
}

void TableDefinitionEditorView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (!m_active) {
        m_selectedSurface = -1;
        m_selectedSide = -1;
        qreal closestDistance = sideSelectionDistance;
        const auto &surfaces = m_definition.surfaces();
        for (qsizetype surfaceIndex = 0; surfaceIndex < surfaces.size(); ++surfaceIndex) {
            const QPolygonF outline = surfaces.at(surfaceIndex).outline();
            for (qsizetype sideIndex = 0; sideIndex < outline.size(); ++sideIndex) {
                const QLineF side(outline.at(sideIndex),
                                  outline.at((sideIndex + 1) % outline.size()));
                const qreal distance = distanceToSegment(event->position(), side);
                if (distance <= closestDistance) {
                    closestDistance = distance;
                    m_selectedSurface = surfaceIndex;
                    m_selectedSide = sideIndex;
                }
            }
        }

        update();
        if (m_selectedSide >= 0) {
            setFocus(Qt::MouseFocusReason);
            event->accept();
            return;
        }
        QWidget::mousePressEvent(event);
        return;
    }

    if (m_draft.isEmpty()) {
        m_draft.append(snappedPoint(event->position()));
    } else if (QLineF(event->position(), m_draft.last()).length()
               > closePointDistance) {
        event->accept();
        return;
    }

    m_drawingSide = true;
    m_cursor = snappedPoint(event->position());
    update();
    event->accept();
}

void TableDefinitionEditorView::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_active || !m_drawingSide) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    m_cursor = snappedPoint(event->position());
    update();
    event->accept();
}

void TableDefinitionEditorView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_active || !m_drawingSide || event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    m_drawingSide = false;
    const QPointF releasedPoint = snappedPoint(event->position());
    if (m_draft.size() >= 3
        && QLineF(releasedPoint, m_draft.first()).length() <= closePointDistance) {
        m_definition.clear();
        m_definition.addSurface(TableSurface(QObject::tr("Table surface"), m_draft));
        reset();
        event->accept();
        emit editingFinished();
        return;
    }

    QLineF side(m_draft.last(), releasedPoint);
    if (side.length() < 1.0) {
        m_cursor = m_draft.last();
        update();
        event->accept();
        return;
    }

    qreal inches = side.length() / (pixelsPerInch * m_viewScale);
    qreal screenAngle = 90.0;
    if (qAbs(side.dx()) >= qAbs(side.dy())) {
        screenAngle = side.dx() >= 0.0 ? 0.0 : 180.0;
    } else {
        screenAngle = side.dy() >= 0.0 ? 90.0 : 270.0;
    }
    if (!getSideMeasurements(this, inches, screenAngle,
                             inches, screenAngle)) {
        m_cursor = m_draft.last();
        update();
        event->accept();
        return;
    }

    const qreal length = inches * pixelsPerInch * m_viewScale;
    const qreal heading = qDegreesToRadians(screenAngle);
    const QPointF newPoint = snappedPoint(
        m_draft.last() + QPointF(length * std::cos(heading),
                                 length * std::sin(heading)));
    m_draft.append(newPoint);
    m_cursor = newPoint;
    update();
    event->accept();
}

void TableDefinitionEditorView::keyPressEvent(QKeyEvent *event)
{
    if (!m_active) {
        if (event->key() == Qt::Key_Delete && m_selectedSurface >= 0
            && m_selectedSide >= 0) {
            const TableSurface &surface =
                m_definition.surfaces().at(m_selectedSurface);
            const QPolygonF outline = surface.outline();
            m_draft.clear();
            const qsizetype firstPoint = (m_selectedSide + 1) % outline.size();
            for (qsizetype offset = 0; offset < outline.size(); ++offset) {
                m_draft.append(outline.at((firstPoint + offset) % outline.size()));
            }

            m_definition.clear();
            m_cursor = m_draft.last();
            m_active = true;
            m_selectedSurface = -1;
            m_selectedSide = -1;
            setCursor(Qt::CrossCursor);
            update();
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
        return;
    }

    if (event->key() == Qt::Key_Escape) {
        reset();
        event->accept();
        emit editingFinished();
        return;
    }

    if (event->key() == Qt::Key_Backspace && !m_draft.isEmpty()) {
        m_draft.removeLast();
        update();
        event->accept();
        return;
    }

    if (event->key() == Qt::Key_Delete) {
        if (m_draft.size() > 1) {
            m_draft.removeLast();
            m_cursor = m_draft.last();
            update();
        }
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void TableDefinitionEditorView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);

    if (m_gridVisible) {
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(205, 205, 205, 150), 1));
        const qreal scaledGridSpacing = gridSpacing * m_viewScale;
        for (qreal x = 0.0; x <= width(); x += scaledGridSpacing) {
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
        }
        for (qreal y = 0.0; y <= height(); y += scaledGridSpacing) {
            painter.drawLine(QPointF(0.0, y), QPointF(width(), y));
        }
    }

    painter.setRenderHint(QPainter::Antialiasing);
    paintDefinition(painter);
}

void TableDefinitionEditorView::paintDefinition(QPainter &painter) const
{
    if (!m_definition.isEmpty()) {
        painter.setPen(QPen(QColor(80, 55, 30), 3));
        painter.setBrush(QColor(181, 143, 92, 120));
        painter.drawPath(m_definition.usableArea());

        painter.setPen(QColor(245, 245, 245));
        for (const TableSurface &surface : m_definition.surfaces()) {
            const QPolygonF outline = surface.outline();
            for (qsizetype index = 0; index < outline.size(); ++index) {
                const QLineF side(outline.at(index),
                                  outline.at((index + 1) % outline.size()));
                painter.drawText(side.center(),
                                 QObject::tr("%1 in").arg(
                                     side.length() / (pixelsPerInch * m_viewScale),
                                     0, 'f', 2));
            }
        }


        if (m_selectedSurface >= 0 && m_selectedSide >= 0) {
            const QPolygonF outline =
                m_definition.surfaces().at(m_selectedSurface).outline();
            painter.setPen(QPen(QColor(255, 80, 70), 6));
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(outline.at(m_selectedSide),
                             outline.at((m_selectedSide + 1) % outline.size()));
        }
    }

    if (!m_active) {
        return;
    }

    painter.setPen(QPen(QColor(255, 190, 70), 2));
    painter.setBrush(QColor(255, 190, 70));
    if (m_draft.size() > 1) {
        painter.drawPolyline(m_draft);
        painter.setPen(QColor(255, 235, 180));
        for (qsizetype index = 1; index < m_draft.size(); ++index) {
            const QLineF side(m_draft.at(index - 1), m_draft.at(index));
            painter.drawText(side.center(),
                             QObject::tr("%1 in").arg(
                                 side.length() / (pixelsPerInch * m_viewScale),
                                 0, 'f', 2));
        }
        painter.setPen(QPen(QColor(255, 190, 70), 2));
    }
    for (const QPointF &point : m_draft) {
        painter.drawEllipse(point, 5, 5);
    }
    if (!m_draft.isEmpty()) {
        painter.setPen(QPen(QColor(255, 220, 130), 1, Qt::DashLine));
        painter.drawLine(m_draft.last(), m_cursor);
        if (m_draft.size() >= 3) {
            painter.drawEllipse(m_draft.first(), closePointDistance,
                                closePointDistance);
        }
    }
}
