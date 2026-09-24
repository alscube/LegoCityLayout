#include "TableDefinitionEditor.h"

#include "TableDefinition.h"
#include "TableSurface.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QKeyEvent>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QWidget>
#include <QtMath>

#include <cmath>

namespace {
constexpr qreal pixelsPerInch = 12.8;
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

TableDefinitionEditor::TableDefinitionEditor(TableDefinition &definition)
    : m_definition(definition)
{
}

void TableDefinitionEditor::begin(QWidget *view)
{
    m_draft.clear();
    m_selectedSurface = -1;
    m_selectedSide = -1;
    m_active = true;
    m_drawingSide = false;
    view->setCursor(Qt::CrossCursor);
    view->setFocus(Qt::OtherFocusReason);
    view->update();
}

void TableDefinitionEditor::reset(QWidget *view)
{
    m_draft.clear();
    m_active = false;
    m_drawingSide = false;
    m_selectedSurface = -1;
    m_selectedSide = -1;
    view->unsetCursor();
    view->update();
}

void TableDefinitionEditor::resetViewScale()
{
    m_viewScale = 1.0;
}

void TableDefinitionEditor::translate(const QPointF &offset)
{
    m_draft.translate(offset);
    m_cursor += offset;
}

void TableDefinitionEditor::scale(const QPointF &anchor, qreal factor)
{
    for (QPointF &point : m_draft) {
        point = anchor + (point - anchor) * factor;
    }
    m_cursor = anchor + (m_cursor - anchor) * factor;
    m_viewScale *= factor;
}

bool TableDefinitionEditor::mousePressEvent(QWidget *view, QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return false;
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

        view->update();
        if (m_selectedSide >= 0) {
            view->setFocus(Qt::MouseFocusReason);
            event->accept();
            return true;
        }
        return false;
    }

    if (m_draft.isEmpty()) {
        m_draft.append(event->position());
    } else if (QLineF(event->position(), m_draft.last()).length()
               > closePointDistance) {
        event->accept();
        return true;
    }

    m_drawingSide = true;
    m_cursor = event->position();
    view->update();
    event->accept();
    return true;
}

bool TableDefinitionEditor::mouseMoveEvent(QWidget *view, QMouseEvent *event)
{
    if (!m_active || !m_drawingSide) {
        return false;
    }

    m_cursor = event->position();
    view->update();
    event->accept();
    return true;
}

bool TableDefinitionEditor::mouseReleaseEvent(QWidget *view, QMouseEvent *event)
{
    if (!m_active || !m_drawingSide || event->button() != Qt::LeftButton) {
        return false;
    }

    m_drawingSide = false;
    const QPointF releasedPoint = event->position();
    if (m_draft.size() >= 3
        && QLineF(releasedPoint, m_draft.first()).length() <= closePointDistance) {
        m_definition.clear();
        m_definition.addSurface(TableSurface(QObject::tr("Table surface"), m_draft));
        reset(view);
        event->accept();
        return true;
    }

    QLineF side(m_draft.last(), releasedPoint);
    if (side.length() < 1.0) {
        m_cursor = m_draft.last();
        view->update();
        event->accept();
        return true;
    }

    qreal inches = side.length() / (pixelsPerInch * m_viewScale);
    qreal screenAngle = 90.0;
    if (qAbs(side.dx()) >= qAbs(side.dy())) {
        screenAngle = side.dx() >= 0.0 ? 0.0 : 180.0;
    } else {
        screenAngle = side.dy() >= 0.0 ? 90.0 : 270.0;
    }
    if (!getSideMeasurements(view, inches, screenAngle,
                             inches, screenAngle)) {
        m_cursor = m_draft.last();
        view->update();
        event->accept();
        return true;
    }

    const qreal length = inches * pixelsPerInch * m_viewScale;
    const qreal heading = qDegreesToRadians(screenAngle);
    const QPointF newPoint = m_draft.last()
                              + QPointF(length * std::cos(heading),
                                        length * std::sin(heading));
    m_draft.append(newPoint);
    m_cursor = newPoint;
    view->update();
    event->accept();
    return true;
}

bool TableDefinitionEditor::keyPressEvent(QWidget *view, QKeyEvent *event)
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
            view->setCursor(Qt::CrossCursor);
            view->update();
            event->accept();
            return true;
        }
        return false;
    }

    if (event->key() == Qt::Key_Escape) {
        reset(view);
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_Backspace && !m_draft.isEmpty()) {
        m_draft.removeLast();
        view->update();
        event->accept();
        return true;
    }

    if (event->key() == Qt::Key_Delete) {
        if (m_draft.size() > 1) {
            m_draft.removeLast();
            m_cursor = m_draft.last();
            view->update();
        }
        event->accept();
        return true;
    }

    return false;
}

void TableDefinitionEditor::paint(QPainter &painter) const
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
