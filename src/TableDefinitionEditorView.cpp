#include "TableDefinitionEditorView.h"

#include "TableDefinition.h"
#include "TableSurface.h"
#include "UserSettings.h"

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

#include <algorithm>
#include <cmath>

namespace {
constexpr qreal gridSizeInches = 10.0;
constexpr qreal gridSizeCentimeters = 25.5;
constexpr qreal gridSpacing = 64.0;
constexpr qreal pixelsPerInch = gridSpacing / gridSizeInches;
constexpr qreal closePointDistance = 14.0;
constexpr qreal endpointSelectionDistance = 7.0;
constexpr qreal sideSelectionDistance = 12.0;
// Use the nominal baseplate dimensions so a grid square is 10 in or 25.5 cm.
constexpr qreal centimetersPerNominalInch =
    gridSizeCentimeters / gridSizeInches;

bool usesMetricMeasurements()
{
    return UserSettings::instance().measurementSystem()
           == UserSettings::MeasurementSystem::Metric;
}

QString formattedLength(qreal inches)
{
    if (usesMetricMeasurements()) {
        return QObject::tr("%1 cm").arg(inches * centimetersPerNominalInch,
                                          0, 'f', 2);
    }
    return QObject::tr("%1 in").arg(inches, 0, 'f', 2);
}

QPointF measurementTextPosition(const QLineF &side)
{
    constexpr qreal labelOffset = 12.0;
    const qreal length = side.length();
    if (qFuzzyIsNull(length)) {
        return side.center();
    }

    QPointF normal(-side.dy() / length, side.dx() / length);
    if (qAbs(side.dx()) >= qAbs(side.dy())) {
        if (normal.y() > 0.0) {
            normal *= -1.0;
        }
    } else if (normal.x() < 0.0) {
        normal *= -1.0;
    }
    return side.center() + normal * labelOffset;
}

qreal screenAngleForSide(const QLineF &side)
{
    qreal angle = qRadiansToDegrees(std::atan2(side.dy(), side.dx()));
    if (angle < 0.0) {
        angle += 360.0;
    }
    return angle;
}

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

bool connectedEndpoints(const QLineF &first, const QLineF &second,
                        QPointF &outerFirst, QPointF &shared,
                        QPointF &outerSecond)
{
    constexpr qreal endpointTolerance = 1.0;
    if (QLineF(first.p2(), second.p1()).length() < endpointTolerance) {
        outerFirst = first.p1();
        shared = first.p2();
        outerSecond = second.p2();
        return true;
    }
    if (QLineF(first.p2(), second.p2()).length() < endpointTolerance) {
        outerFirst = first.p1();
        shared = first.p2();
        outerSecond = second.p1();
        return true;
    }
    if (QLineF(first.p1(), second.p1()).length() < endpointTolerance) {
        outerFirst = first.p2();
        shared = first.p1();
        outerSecond = second.p2();
        return true;
    }
    if (QLineF(first.p1(), second.p2()).length() < endpointTolerance) {
        outerFirst = first.p2();
        shared = first.p1();
        outerSecond = second.p1();
        return true;
    }
    return false;
}

void mergeConnectedCollinearSides(QList<QLineF> &sides)
{
    bool merged = true;
    while (merged) {
        merged = false;
        for (qsizetype firstIndex = 0; firstIndex < sides.size(); ++firstIndex) {
            for (qsizetype secondIndex = firstIndex + 1;
                 secondIndex < sides.size(); ++secondIndex) {
                QPointF outerFirst;
                QPointF shared;
                QPointF outerSecond;
                if (!connectedEndpoints(sides.at(firstIndex),
                                        sides.at(secondIndex),
                                        outerFirst, shared, outerSecond)) {
                    continue;
                }

                const QPointF incoming = shared - outerFirst;
                const QPointF outgoing = outerSecond - shared;
                const qreal lengthProduct =
                    QLineF(QPointF(), incoming).length()
                    * QLineF(QPointF(), outgoing).length();
                if (qFuzzyIsNull(lengthProduct)) {
                    continue;
                }

                const qreal cross = incoming.x() * outgoing.y()
                                    - incoming.y() * outgoing.x();
                const qreal dot = incoming.x() * outgoing.x()
                                  + incoming.y() * outgoing.y();
                if (qAbs(cross) > lengthProduct * 0.000001 || dot <= 0.0) {
                    continue;
                }

                sides[firstIndex] = QLineF(outerFirst, outerSecond);
                sides.removeAt(secondIndex);
                merged = true;
                break;
            }
            if (merged) {
                break;
            }
        }
    }
}

void removeCollinearOutlinePoints(QPolygonF &outline)
{
    bool removed = true;
    while (removed && outline.size() >= 3) {
        removed = false;
        for (qsizetype index = 0; index < outline.size(); ++index) {
            const QPointF incoming = outline.at(index)
                                     - outline.at((index + outline.size() - 1)
                                                  % outline.size());
            const QPointF outgoing = outline.at((index + 1) % outline.size())
                                     - outline.at(index);
            const qreal lengthProduct =
                QLineF(QPointF(), incoming).length()
                * QLineF(QPointF(), outgoing).length();
            if (qFuzzyIsNull(lengthProduct)) {
                continue;
            }

            const qreal cross = incoming.x() * outgoing.y()
                                - incoming.y() * outgoing.x();
            const qreal dot = incoming.x() * outgoing.x()
                              + incoming.y() * outgoing.y();
            if (qAbs(cross) <= lengthProduct * 0.000001 && dot > 0.0) {
                outline.removeAt(index);
                removed = true;
                break;
            }
        }
    }
}

bool getSideMeasurements(QWidget *parent, qreal suggestedLength,
                         qreal suggestedAngle,
                         qreal &length, qreal &angle)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(QObject::tr("Side Measurement"));

    auto *layout = new QFormLayout(&dialog);
    auto *lengthInput = new QDoubleSpinBox(&dialog);
    const bool metric = usesMetricMeasurements();
    const qreal displayScale = metric ? centimetersPerNominalInch : 1.0;
    lengthInput->setRange(0.1, 1000.0 * displayScale);
    lengthInput->setDecimals(2);
    lengthInput->setSuffix(metric ? QObject::tr(" cm") : QObject::tr(" in"));
    lengthInput->setValue(suggestedLength * displayScale);

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

    length = lengthInput->value() / displayScale;
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

    m_unitsLabel = new QLabel(header);
    refreshMeasurementUnits();
    headerLayout->addWidget(m_unitsLabel);

    auto *gridButton = new QPushButton(tr("Hide Grid"), header);
    gridButton->setCheckable(true);
    gridButton->setChecked(true);
    gridButton->setToolTip(tr("Show or hide the measurement grid"));
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
        tr("Press and drag to draw a side.  Connect points to close.  "
            "Select line and press Delete key to remove."), this);
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
    m_editSides = m_definition.openSides();
    m_draftSurfaceName.clear();
    m_selectedSurface = -1;
    m_selectedSide = -1;
    m_active = m_definition.isEmpty() || !m_editSides.isEmpty();
    m_drawingSide = false;
    m_editingIndividualSides = !m_editSides.isEmpty();
    if (m_active) {
        setCursor(Qt::CrossCursor);
    } else {
        unsetCursor();
    }
    setFocus(Qt::OtherFocusReason);
    update();
}

void TableDefinitionEditorView::reset()
{
    m_draft.clear();
    m_editSides.clear();
    m_draftSurfaceName.clear();
    m_active = false;
    m_drawingSide = false;
    m_editingIndividualSides = false;
    m_selectedSurface = -1;
    m_selectedSide = -1;
    unsetCursor();
    update();
}

void TableDefinitionEditorView::resetViewScale()
{
    m_viewScale = 1.0;
}

void TableDefinitionEditorView::refreshMeasurementUnits()
{
    const QString units = usesMetricMeasurements()
                              ? tr("Centimeters")
                              : tr("Inches");
    m_unitsLabel->setText(tr("Units: %1").arg(units));
    update();
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

    if (m_editingIndividualSides) {
        qreal closestEndpointDistance = endpointSelectionDistance;
        bool endpointFound = false;
        QPointF closestEndpoint;
        for (const QLineF &side : m_editSides) {
            for (const QPointF &endpoint : {side.p1(), side.p2()}) {
                const qreal distance =
                    QLineF(event->position(), endpoint).length();
                if (distance <= closestEndpointDistance) {
                    closestEndpointDistance = distance;
                    closestEndpoint = endpoint;
                    endpointFound = true;
                }
            }
        }

        if (endpointFound) {
            m_selectedSide = -1;
            m_sideStart = closestEndpoint;
            m_cursor = closestEndpoint;
            m_drawingSide = true;
            setFocus(Qt::MouseFocusReason);
            update();
            event->accept();
            return;
        }

        m_selectedSide = -1;
        qreal closestDistance = sideSelectionDistance;
        for (qsizetype index = 0; index < m_editSides.size(); ++index) {
            const qreal distance =
                distanceToSegment(event->position(), m_editSides.at(index));
            if (distance <= closestDistance) {
                closestDistance = distance;
                m_selectedSide = index;
            }
        }

        if (m_selectedSide >= 0) {
            setFocus(Qt::MouseFocusReason);
            update();
            event->accept();
            return;
        }

        m_sideStart = snappedPoint(event->position());
        m_cursor = m_sideStart;
        m_drawingSide = true;
        update();
        event->accept();
        return;
    }

    if (m_draft.isEmpty()) {
        m_draft.append(snappedPoint(event->position()));
    } else {
        const qreal distanceToLast =
            QLineF(event->position(), m_draft.last()).length();
        const qreal distanceToFirst =
            QLineF(event->position(), m_draft.first()).length();

        if (distanceToLast > endpointSelectionDistance
            && distanceToFirst > endpointSelectionDistance) {
            qsizetype selectedDraftSide = -1;
            qreal closestDistance = sideSelectionDistance;
            for (qsizetype index = 1; index < m_draft.size(); ++index) {
                const QLineF side(m_draft.at(index - 1), m_draft.at(index));
                const qreal distance =
                    distanceToSegment(event->position(), side);
                if (distance <= closestDistance) {
                    closestDistance = distance;
                    selectedDraftSide = index - 1;
                }
            }

            if (selectedDraftSide >= 0) {
                m_editSides.clear();
                for (qsizetype index = 1; index < m_draft.size(); ++index) {
                    m_editSides.append(
                        QLineF(m_draft.at(index - 1), m_draft.at(index)));
                }
                m_draft.clear();
                m_editingIndividualSides = true;
                m_selectedSide = selectedDraftSide;
                syncOpenSides();
                setFocus(Qt::MouseFocusReason);
                update();
                event->accept();
                return;
            }
        }

        if (distanceToLast > endpointSelectionDistance) {
            if (distanceToFirst <= endpointSelectionDistance) {
                std::reverse(m_draft.begin(), m_draft.end());
            } else {
                event->accept();
                return;
            }
        }
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
    if (m_editingIndividualSides) {
        QPointF sideEnd = releasedPoint;
        qreal closestEndpointDistance = closePointDistance;
        for (const QLineF &existingSide : m_editSides) {
            for (const QPointF &endpoint : {existingSide.p1(), existingSide.p2()}) {
                const qreal distance = QLineF(sideEnd, endpoint).length();
                if (distance <= closestEndpointDistance) {
                    closestEndpointDistance = distance;
                    sideEnd = endpoint;
                }
            }
        }

        QLineF side(m_sideStart, sideEnd);
        if (side.length() < 1.0) {
            update();
            event->accept();
            return;
        }

        qreal inches = side.length() / (pixelsPerInch * m_viewScale);
        qreal screenAngle = screenAngleForSide(side);
        if (!getSideMeasurements(this, inches, screenAngle,
                                 inches, screenAngle)) {
            update();
            event->accept();
            return;
        }

        const qreal length = inches * pixelsPerInch * m_viewScale;
        const qreal heading = qDegreesToRadians(screenAngle);
        const QPointF endPoint = snappedPoint(
            m_sideStart + QPointF(length * std::cos(heading),
                                  length * std::sin(heading)));
        m_editSides.append(QLineF(m_sideStart, endPoint));
        mergeConnectedCollinearSides(m_editSides);
        syncOpenSides();
        finishSideEditingIfClosed();
        update();
        event->accept();
        return;
    }
    if (m_draft.size() >= 3
        && QLineF(releasedPoint, m_draft.first()).length() <= closePointDistance) {
        removeCollinearOutlinePoints(m_draft);
        const QString surfaceName = m_draftSurfaceName.isEmpty()
                                        ? QObject::tr("Table surface")
                                        : m_draftSurfaceName;
        m_definition.addSurface(TableSurface(surfaceName, m_draft));
        m_definition.setOpenSides({});
        reset();
        event->accept();
        emit editingFinished();
        return;
    }

    QLineF side(m_draft.last(), releasedPoint);
    if (side.length() < 1.0) {
        if (m_draft.size() == 1) {
            m_draft.clear();
        } else {
            m_cursor = m_draft.last();
        }
        update();
        event->accept();
        return;
    }

    qreal inches = side.length() / (pixelsPerInch * m_viewScale);
    qreal screenAngle = screenAngleForSide(side);
    if (!getSideMeasurements(this, inches, screenAngle,
                             inches, screenAngle)) {
        if (m_draft.size() == 1) {
            m_draft.clear();
        } else {
            m_cursor = m_draft.last();
        }
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
    removeCollinearOutlinePoints(m_draft);
    syncOpenSides();
    m_cursor = newPoint;
    update();
    event->accept();
}

void TableDefinitionEditorView::keyPressEvent(QKeyEvent *event)
{
    int keyPressed = event->key();

    if (!m_active) {
        if ((keyPressed == Qt::Key_Delete || keyPressed == Qt::Key_Backspace)
            && m_selectedSurface >= 0
            && m_selectedSide >= 0) {
            deleteSelectedSide();
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
        return;
    }

    if (m_editingIndividualSides
        && (keyPressed == Qt::Key_Delete || keyPressed == Qt::Key_Backspace)
        && m_selectedSide >= 0) {
        deleteSelectedSide();
        event->accept();
        return;
    }

    if (keyPressed == Qt::Key_Escape) {
        syncOpenSides();
        reset();
        event->accept();
        emit editingFinished();
        return;
    }

    if (keyPressed == Qt::Key_Backspace && !m_draft.isEmpty()) {
        m_draft.removeLast();
        if (m_draft.size() == 1) {
            m_draft.clear();
        } else if (!m_draft.isEmpty()) {
            m_cursor = m_draft.last();
        }
        syncOpenSides();
        update();
        event->accept();
        return;
    }

    if (keyPressed == Qt::Key_Delete) {
        if (m_draft.size() > 1) {
            m_draft.removeLast();
            if (m_draft.size() == 1) {
                m_draft.clear();
            } else {
                m_cursor = m_draft.last();
            }
            syncOpenSides();
            update();
        }
        event->accept();
        return;
    }

    QWidget::keyPressEvent(event);
}

void TableDefinitionEditorView::deleteSelectedSide()
{
    if (m_editingIndividualSides) {
        if (m_selectedSide >= 0 && m_selectedSide < m_editSides.size()) {
            m_editSides.removeAt(m_selectedSide);
            syncOpenSides();
            m_selectedSide = -1;
            update();
        }
        return;
    }

    if (m_active || m_selectedSurface < 0 || m_selectedSide < 0) {
        return;
    }

    const TableSurface &surface =
        m_definition.surfaces().at(m_selectedSurface);
    const QPolygonF outline = surface.outline();
    m_draftSurfaceName = surface.name();
    m_editSides.clear();
    for (qsizetype index = 0; index < outline.size(); ++index) {
        if (index != m_selectedSide) {
            m_editSides.append(
                QLineF(outline.at(index),
                       outline.at((index + 1) % outline.size())));
        }
    }
    mergeConnectedCollinearSides(m_editSides);

    m_definition.removeSurface(m_selectedSurface);
    m_active = true;
    m_editingIndividualSides = true;
    syncOpenSides();
    m_selectedSurface = -1;
    m_selectedSide = -1;
    setCursor(Qt::CrossCursor);
    setFocus(Qt::OtherFocusReason);
    update();
}

void TableDefinitionEditorView::finishSideEditingIfClosed()
{
    if (m_editSides.size() < 3) {
        return;
    }

    QList<bool> used(m_editSides.size(), false);
    QPolygonF outline;
    outline.append(m_editSides.first().p1());
    QPointF current = m_editSides.first().p2();
    outline.append(current);
    used[0] = true;

    for (qsizetype count = 1; count < m_editSides.size(); ++count) {
        qsizetype nextIndex = -1;
        QPointF nextPoint;
        for (qsizetype index = 0; index < m_editSides.size(); ++index) {
            if (used.at(index)) {
                continue;
            }

            const QLineF &candidate = m_editSides.at(index);
            if (QLineF(current, candidate.p1()).length() < 1.0) {
                nextIndex = index;
                nextPoint = candidate.p2();
                break;
            }
            if (QLineF(current, candidate.p2()).length() < 1.0) {
                nextIndex = index;
                nextPoint = candidate.p1();
                break;
            }
        }

        if (nextIndex < 0) {
            return;
        }
        used[nextIndex] = true;
        current = nextPoint;
        outline.append(current);
    }

    if (QLineF(outline.last(), outline.first()).length() >= 1.0) {
        return;
    }
    outline.removeLast();

    const QString surfaceName = m_draftSurfaceName.isEmpty()
                                    ? tr("Table surface")
                                    : m_draftSurfaceName;
    if (!m_definition.addSurface(TableSurface(surfaceName, outline))) {
        return;
    }

    m_definition.setOpenSides({});
    reset();
    emit editingFinished();
}

void TableDefinitionEditorView::syncOpenSides()
{
    if (m_editingIndividualSides) {
        m_definition.setOpenSides(m_editSides);
        return;
    }

    QList<QLineF> sides;
    for (qsizetype index = 1; index < m_draft.size(); ++index) {
        sides.append(QLineF(m_draft.at(index - 1), m_draft.at(index)));
    }
    m_definition.setOpenSides(sides);
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
    const QColor editingLineColor(105, 70, 30);
    const QColor editingMeasurementColor(75, 50, 25);
    const QColor editingPreviewColor(130, 90, 40);

    if (!m_definition.isEmpty()) {
        painter.setPen(QPen(QColor(80, 55, 30), 3));
        painter.setBrush(QColor(181, 143, 92, 120));
        painter.drawPath(m_definition.usableArea());

        painter.setPen(editingMeasurementColor);
        for (const TableSurface &surface : m_definition.surfaces()) {
            const QPolygonF outline = surface.outline();
            for (qsizetype index = 0; index < outline.size(); ++index) {
                const QLineF side(outline.at(index),
                                  outline.at((index + 1) % outline.size()));
                painter.drawText(
                    measurementTextPosition(side),
                    formattedLength(
                        side.length() / (pixelsPerInch * m_viewScale)));
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

    if (m_editingIndividualSides) {
        painter.setBrush(editingLineColor);
        for (qsizetype index = 0; index < m_editSides.size(); ++index) {
            const QLineF &side = m_editSides.at(index);
            painter.setPen(index == m_selectedSide
                               ? QPen(QColor(255, 80, 70), 6)
                               : QPen(editingLineColor, 2));
            painter.drawLine(side);
            painter.drawEllipse(side.p1(), 5, 5);
            painter.drawEllipse(side.p2(), 5, 5);
            painter.setPen(editingMeasurementColor);
            painter.drawText(
                measurementTextPosition(side),
                formattedLength(
                    side.length() / (pixelsPerInch * m_viewScale)));
        }
        if (m_drawingSide) {
            painter.setPen(QPen(editingPreviewColor, 1, Qt::DashLine));
            painter.drawLine(m_sideStart, m_cursor);
        }
        return;
    }

    painter.setPen(QPen(editingLineColor, 2));
    painter.setBrush(editingLineColor);
    if (m_draft.size() > 1) {
        painter.drawPolyline(m_draft);
        painter.setPen(editingMeasurementColor);
        for (qsizetype index = 1; index < m_draft.size(); ++index) {
            const QLineF side(m_draft.at(index - 1), m_draft.at(index));
            painter.drawText(
                measurementTextPosition(side),
                formattedLength(
                    side.length() / (pixelsPerInch * m_viewScale)));
        }
        painter.setPen(QPen(editingLineColor, 2));
    }
    for (const QPointF &point : m_draft) {
        painter.drawEllipse(point, 5, 5);
    }
    if (!m_draft.isEmpty()) {
        painter.setPen(QPen(editingPreviewColor, 1, Qt::DashLine));
        painter.drawLine(m_draft.last(), m_cursor);
        if (m_draft.size() >= 3) {
            painter.drawEllipse(m_draft.first(), closePointDistance,
                                closePointDistance);
        }
    }
}
