
#include <QSignalBlocker>
#include "TableDefinitionEditorView.h"

#include "TableDefinition.h"
#include "TableSurface.h"
#include "CityLayoutElement.h"
#include "LoadedProjects.h"
#include "UserSettings.h"
#include "LegoGrid.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFocusEvent>
#include <QFormLayout>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineF>
#include <QMouseEvent>
#include <QPainter>
#include <QPushButton>
#include <QWidget>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {
constexpr qreal gridSpacing = LegoGrid::plateSpacing;
constexpr qreal pixelsPerInch = LegoGrid::pixelsPerInch;
constexpr qreal closePointDistance = 14.0;
constexpr qreal endpointSelectionDistance = 7.0;
constexpr qreal sideSelectionDistance = 12.0;
QString formattedLength(qreal inches)
{
    const auto &settings = UserSettings::instance();
    return QObject::tr("%1 %2")
        .arg(inches * settings.measurementUnitsPerInch(), 0, 'f', 2)
        .arg(settings.measurementAbbreviation());
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
    const auto &settings = UserSettings::instance();
    const qreal displayScale = settings.measurementUnitsPerInch();
    lengthInput->setRange(0.1 * displayScale, 1000.0 * displayScale);
    lengthInput->setDecimals(2);
    lengthInput->setSuffix(QStringLiteral(" ") + settings.measurementAbbreviation());
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

TableDefinitionEditorView::TableDefinitionEditorView(QWidget *parent)
    : QWidget(parent)
    , _Projects(LoadedProjects::instance())
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

    layout->addWidget(header);
    layout->addStretch();

    auto *instructions = new QLabel(
        tr("Press and drag to draw a side.  Connect points to close.  "
            "Select line and press Delete key to remove.  "
            "Middle-drag or Space + drag to pan.  Scroll to zoom."), this);
    instructions->setAlignment(Qt::AlignCenter);
    instructions->setContentsMargins(12, 10, 12, 10);
    instructions->setStyleSheet(
        "QLabel { background-color: palette(window); "
        "border-top: 1px solid palette(mid); font-weight: 600; }");
    layout->addWidget(instructions);
}

void TableDefinitionEditorView::activateProject()
{
    if (!_Projects.currentProject()) return;
    m_panButton = Qt::NoButton;
    m_spaceHeld = false;
    if (_Projects.tableEditor().active) setCursor(Qt::CrossCursor);
    else unsetCursor();
    update();
}

void TableDefinitionEditorView::begin()
{
    _Projects.tableEditor().draft.clear();
    _Projects.tableEditor().editSides = _Projects.tableDefinition().openSides();
    _Projects.tableEditor().draftSurfaceName.clear();
    _Projects.tableEditor().selectedSurface = -1;
    _Projects.tableEditor().selectedSide = -1;
    _Projects.tableEditor().active = _Projects.tableDefinition().isEmpty() || !_Projects.tableEditor().editSides.isEmpty();
    _Projects.tableEditor().drawingSide = false;
    _Projects.tableEditor().editingIndividualSides = !_Projects.tableEditor().editSides.isEmpty();
    if (_Projects.tableEditor().active) {
        setCursor(Qt::CrossCursor);
    } else {
        unsetCursor();
    }
    setFocus(Qt::OtherFocusReason);
    update();
}

void TableDefinitionEditorView::reset()
{
    m_panButton = Qt::NoButton;
    m_spaceHeld = false;
    _Projects.tableEditor().draft.clear();
    _Projects.tableEditor().editSides.clear();
    _Projects.tableEditor().draftSurfaceName.clear();
    _Projects.tableEditor().active = false;
    _Projects.tableEditor().drawingSide = false;
    _Projects.tableEditor().editingIndividualSides = false;
    _Projects.tableEditor().selectedSurface = -1;
    _Projects.tableEditor().selectedSide = -1;
    unsetCursor();
    update();
}

void TableDefinitionEditorView::resetViewScale()
{
    _Projects.setViewZoom(rect().center(), 1.0);
    update();
}

void TableDefinitionEditorView::refreshMeasurementUnits()
{
    if (_Projects.currentProject() && _Projects.tableEditor().drawingSide)
        _Projects.tableEditor().cursor = snappedPoint(_Projects.tableEditor().cursor);
    emit measurementUnitsChanged();
    update();
}

QPointF TableDefinitionEditorView::snappedPoint(const QPointF &point) const
{
    if (!_Projects.tableEditor().snapToGrid) {
        return point;
    }

    // Table outlines snap to the visible baseplate grid; city elements snap to studs.
    const qreal spacing = gridSpacing * _Projects.tableEditor().viewScale;
    const QPointF origin = _Projects.currentProject()->gridOrigin;
    const QPointF relative = point - origin;
    return origin + QPointF(qRound(relative.x() / spacing) * spacing,
                            qRound(relative.y() / spacing) * spacing);
}

void TableDefinitionEditorView::mousePressEvent(QMouseEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    if (!_Projects.tableEditor().drawingSide
        && (event->button() == Qt::MiddleButton
            || (event->button() == Qt::LeftButton && m_spaceHeld))) {
        m_panButton = event->button();
        m_lastPanPosition = event->position();
        setFocus(Qt::MouseFocusReason);
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (!_Projects.tableEditor().active) {
        _Projects.tableEditor().selectedSurface = -1;
        _Projects.tableEditor().selectedSide = -1;
        qreal closestDistance = sideSelectionDistance;
        const auto &surfaces = _Projects.tableDefinition().surfaces();
        for (qsizetype surfaceIndex = 0; surfaceIndex < surfaces.size(); ++surfaceIndex) {
            const QPolygonF outline = surfaces.at(surfaceIndex).outline();
            for (qsizetype sideIndex = 0; sideIndex < outline.size(); ++sideIndex) {
                const QLineF side(outline.at(sideIndex),
                                  outline.at((sideIndex + 1) % outline.size()));
                const qreal distance = distanceToSegment(event->position(), side);
                if (distance <= closestDistance) {
                    closestDistance = distance;
                    _Projects.tableEditor().selectedSurface = surfaceIndex;
                    _Projects.tableEditor().selectedSide = sideIndex;
                }
            }
        }

        update();
        if (_Projects.tableEditor().selectedSide >= 0) {
            setFocus(Qt::MouseFocusReason);
            event->accept();
            return;
        }
        m_panButton = Qt::LeftButton;
        m_lastPanPosition = event->position();
        setFocus(Qt::MouseFocusReason);
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (_Projects.tableEditor().editingIndividualSides) {
        qreal closestEndpointDistance = endpointSelectionDistance;
        bool endpointFound = false;
        QPointF closestEndpoint;
        for (const QLineF &side : _Projects.tableEditor().editSides) {
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
            _Projects.tableEditor().selectedSide = -1;
            _Projects.tableEditor().sideStart = closestEndpoint;
            _Projects.tableEditor().cursor = closestEndpoint;
            _Projects.tableEditor().drawingSide = true;
            setFocus(Qt::MouseFocusReason);
            update();
            event->accept();
            return;
        }

        _Projects.tableEditor().selectedSide = -1;
        qreal closestDistance = sideSelectionDistance;
        for (qsizetype index = 0; index < _Projects.tableEditor().editSides.size(); ++index) {
            const qreal distance =
                distanceToSegment(event->position(), _Projects.tableEditor().editSides.at(index));
            if (distance <= closestDistance) {
                closestDistance = distance;
                _Projects.tableEditor().selectedSide = index;
            }
        }

        if (_Projects.tableEditor().selectedSide >= 0) {
            setFocus(Qt::MouseFocusReason);
            update();
            event->accept();
            return;
        }

        _Projects.tableEditor().sideStart = snappedPoint(event->position());
        _Projects.tableEditor().cursor = _Projects.tableEditor().sideStart;
        _Projects.tableEditor().drawingSide = true;
        update();
        event->accept();
        return;
    }

    if (_Projects.tableEditor().draft.isEmpty()) {
        _Projects.tableEditor().draft.append(snappedPoint(event->position()));
    } else {
        const qreal distanceToLast =
            QLineF(event->position(), _Projects.tableEditor().draft.last()).length();
        const qreal distanceToFirst =
            QLineF(event->position(), _Projects.tableEditor().draft.first()).length();

        if (distanceToLast > endpointSelectionDistance
            && distanceToFirst > endpointSelectionDistance) {
            qsizetype selectedDraftSide = -1;
            qreal closestDistance = sideSelectionDistance;
            for (qsizetype index = 1; index < _Projects.tableEditor().draft.size(); ++index) {
                const QLineF side(_Projects.tableEditor().draft.at(index - 1), _Projects.tableEditor().draft.at(index));
                const qreal distance =
                    distanceToSegment(event->position(), side);
                if (distance <= closestDistance) {
                    closestDistance = distance;
                    selectedDraftSide = index - 1;
                }
            }

            if (selectedDraftSide >= 0) {
                _Projects.tableEditor().editSides.clear();
                for (qsizetype index = 1; index < _Projects.tableEditor().draft.size(); ++index) {
                    _Projects.tableEditor().editSides.append(
                        QLineF(_Projects.tableEditor().draft.at(index - 1), _Projects.tableEditor().draft.at(index)));
                }
                _Projects.tableEditor().draft.clear();
                _Projects.tableEditor().editingIndividualSides = true;
                _Projects.tableEditor().selectedSide = selectedDraftSide;
                syncOpenSides();
                setFocus(Qt::MouseFocusReason);
                update();
                event->accept();
                return;
            }
        }

        if (distanceToLast > endpointSelectionDistance) {
            if (distanceToFirst <= endpointSelectionDistance) {
                std::reverse(_Projects.tableEditor().draft.begin(), _Projects.tableEditor().draft.end());
            } else {
                event->accept();
                return;
            }
        }
    }

    _Projects.tableEditor().drawingSide = true;
    _Projects.tableEditor().cursor = snappedPoint(event->position());
    update();
    event->accept();
}

void TableDefinitionEditorView::mouseMoveEvent(QMouseEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    if (m_panButton != Qt::NoButton) {
        if (event->buttons().testFlag(m_panButton)) {
            panBy(event->position() - m_lastPanPosition);
            m_lastPanPosition = event->position();
            event->accept();
            return;
        }
        stopPanning();
    }
    if (!_Projects.tableEditor().active || !_Projects.tableEditor().drawingSide) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    _Projects.tableEditor().cursor = snappedPoint(event->position());
    update();
    event->accept();
}

void TableDefinitionEditorView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    if (m_panButton != Qt::NoButton && event->button() == m_panButton) {
        stopPanning();
        event->accept();
        return;
    }
    if (!_Projects.tableEditor().active || !_Projects.tableEditor().drawingSide || event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    _Projects.tableEditor().drawingSide = false;
    const QPointF releasedPoint = snappedPoint(event->position());
    if (_Projects.tableEditor().editingIndividualSides) {
        QPointF sideEnd = releasedPoint;
        qreal closestEndpointDistance = closePointDistance;
        for (const QLineF &existingSide : _Projects.tableEditor().editSides) {
            for (const QPointF &endpoint : {existingSide.p1(), existingSide.p2()}) {
                const qreal distance = QLineF(sideEnd, endpoint).length();
                if (distance <= closestEndpointDistance) {
                    closestEndpointDistance = distance;
                    sideEnd = endpoint;
                }
            }
        }

        QLineF side(_Projects.tableEditor().sideStart, sideEnd);
        if (side.length() < 1.0) {
            update();
            event->accept();
            return;
        }

        qreal inches = side.length() / (pixelsPerInch * _Projects.tableEditor().viewScale);
        qreal screenAngle = screenAngleForSide(side);
        if (!getSideMeasurements(this, inches, screenAngle,
                                 inches, screenAngle)) {
            update();
            event->accept();
            return;
        }

        const qreal length = inches * pixelsPerInch * _Projects.tableEditor().viewScale;
        const qreal heading = qDegreesToRadians(screenAngle);
        const QPointF endPoint = snappedPoint(
            _Projects.tableEditor().sideStart + QPointF(length * std::cos(heading),
                                  length * std::sin(heading)));
        _Projects.tableEditor().editSides.append(QLineF(_Projects.tableEditor().sideStart, endPoint));
        mergeConnectedCollinearSides(_Projects.tableEditor().editSides);
        syncOpenSides();
        finishSideEditingIfClosed();
        update();
        event->accept();
        return;
    }
    if (_Projects.tableEditor().draft.size() >= 3
        && QLineF(releasedPoint, _Projects.tableEditor().draft.first()).length() <= closePointDistance) {
        removeCollinearOutlinePoints(_Projects.tableEditor().draft);
        const QString surfaceName = _Projects.tableEditor().draftSurfaceName.isEmpty()
                                        ? QObject::tr("Table surface")
                                        : _Projects.tableEditor().draftSurfaceName;
        _Projects.tableDefinition().addSurface(TableSurface(surfaceName, _Projects.tableEditor().draft));
        _Projects.tableDefinition().setOpenSides({});
        reset();
        event->accept();
        emit editingFinished();
        return;
    }

    QLineF side(_Projects.tableEditor().draft.last(), releasedPoint);
    if (side.length() < 1.0) {
        if (_Projects.tableEditor().draft.size() == 1) {
            _Projects.tableEditor().draft.clear();
        } else {
            _Projects.tableEditor().cursor = _Projects.tableEditor().draft.last();
        }
        update();
        event->accept();
        return;
    }

    qreal inches = side.length() / (pixelsPerInch * _Projects.tableEditor().viewScale);
    qreal screenAngle = screenAngleForSide(side);
    if (!getSideMeasurements(this, inches, screenAngle,
                             inches, screenAngle)) {
        if (_Projects.tableEditor().draft.size() == 1) {
            _Projects.tableEditor().draft.clear();
        } else {
            _Projects.tableEditor().cursor = _Projects.tableEditor().draft.last();
        }
        update();
        event->accept();
        return;
    }

    const qreal length = inches * pixelsPerInch * _Projects.tableEditor().viewScale;
    const qreal heading = qDegreesToRadians(screenAngle);
    const QPointF newPoint = snappedPoint(
        _Projects.tableEditor().draft.last() + QPointF(length * std::cos(heading),
                                 length * std::sin(heading)));
    _Projects.tableEditor().draft.append(newPoint);
    removeCollinearOutlinePoints(_Projects.tableEditor().draft);
    syncOpenSides();
    _Projects.tableEditor().cursor = newPoint;
    update();
    event->accept();
}

void TableDefinitionEditorView::keyPressEvent(QKeyEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    int keyPressed = event->key();
    if (keyPressed == Qt::Key_Space) {
        m_spaceHeld = true;
        if (m_panButton == Qt::NoButton) setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }

    if (!_Projects.tableEditor().active) {
        if ((keyPressed == Qt::Key_Delete || keyPressed == Qt::Key_Backspace)
            && _Projects.tableEditor().selectedSurface >= 0
            && _Projects.tableEditor().selectedSide >= 0) {
            deleteSelectedSide();
            event->accept();
            return;
        }
        QWidget::keyPressEvent(event);
        return;
    }

    if (_Projects.tableEditor().editingIndividualSides
        && (keyPressed == Qt::Key_Delete || keyPressed == Qt::Key_Backspace)
        && _Projects.tableEditor().selectedSide >= 0) {
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

    if (keyPressed == Qt::Key_Backspace && !_Projects.tableEditor().draft.isEmpty()) {
        _Projects.tableEditor().draft.removeLast();
        if (_Projects.tableEditor().draft.size() == 1) {
            _Projects.tableEditor().draft.clear();
        } else if (!_Projects.tableEditor().draft.isEmpty()) {
            _Projects.tableEditor().cursor = _Projects.tableEditor().draft.last();
        }
        syncOpenSides();
        update();
        event->accept();
        return;
    }

    if (keyPressed == Qt::Key_Delete) {
        if (_Projects.tableEditor().draft.size() > 1) {
            _Projects.tableEditor().draft.removeLast();
            if (_Projects.tableEditor().draft.size() == 1) {
                _Projects.tableEditor().draft.clear();
            } else {
                _Projects.tableEditor().cursor = _Projects.tableEditor().draft.last();
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
    if (_Projects.tableEditor().editingIndividualSides) {
        if (_Projects.tableEditor().selectedSide >= 0 && _Projects.tableEditor().selectedSide < _Projects.tableEditor().editSides.size()) {
            _Projects.tableEditor().editSides.removeAt(_Projects.tableEditor().selectedSide);
            syncOpenSides();
            _Projects.tableEditor().selectedSide = -1;
            update();
        }
        return;
    }

    if (_Projects.tableEditor().active || _Projects.tableEditor().selectedSurface < 0 || _Projects.tableEditor().selectedSide < 0) {
        return;
    }

    const TableSurface &surface =
        _Projects.tableDefinition().surfaces().at(_Projects.tableEditor().selectedSurface);
    const QPolygonF outline = surface.outline();
    _Projects.tableEditor().draftSurfaceName = surface.name();
    _Projects.tableEditor().editSides.clear();
    for (qsizetype index = 0; index < outline.size(); ++index) {
        if (index != _Projects.tableEditor().selectedSide) {
            _Projects.tableEditor().editSides.append(
                QLineF(outline.at(index),
                       outline.at((index + 1) % outline.size())));
        }
    }
    mergeConnectedCollinearSides(_Projects.tableEditor().editSides);

    _Projects.tableDefinition().removeSurface(_Projects.tableEditor().selectedSurface);
    _Projects.tableEditor().active = true;
    _Projects.tableEditor().editingIndividualSides = true;
    syncOpenSides();
    _Projects.tableEditor().selectedSurface = -1;
    _Projects.tableEditor().selectedSide = -1;
    setCursor(Qt::CrossCursor);
    setFocus(Qt::OtherFocusReason);
    update();
}

void TableDefinitionEditorView::finishSideEditingIfClosed()
{
    if (_Projects.tableEditor().editSides.size() < 3) {
        return;
    }

    QList<bool> used(_Projects.tableEditor().editSides.size(), false);
    QPolygonF outline;
    outline.append(_Projects.tableEditor().editSides.first().p1());
    QPointF current = _Projects.tableEditor().editSides.first().p2();
    outline.append(current);
    used[0] = true;

    for (qsizetype count = 1; count < _Projects.tableEditor().editSides.size(); ++count) {
        qsizetype nextIndex = -1;
        QPointF nextPoint;
        for (qsizetype index = 0; index < _Projects.tableEditor().editSides.size(); ++index) {
            if (used.at(index)) {
                continue;
            }

            const QLineF &candidate = _Projects.tableEditor().editSides.at(index);
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

    const QString surfaceName = _Projects.tableEditor().draftSurfaceName.isEmpty()
                                    ? tr("Table surface")
                                    : _Projects.tableEditor().draftSurfaceName;
    if (!_Projects.tableDefinition().addSurface(TableSurface(surfaceName, outline))) {
        return;
    }

    _Projects.tableDefinition().setOpenSides({});
    reset();
    emit editingFinished();
}

void TableDefinitionEditorView::syncOpenSides()
{
    if (_Projects.tableEditor().editingIndividualSides) {
        _Projects.tableDefinition().setOpenSides(_Projects.tableEditor().editSides);
        return;
    }

    QList<QLineF> sides;
    for (qsizetype index = 1; index < _Projects.tableEditor().draft.size(); ++index) {
        sides.append(QLineF(_Projects.tableEditor().draft.at(index - 1), _Projects.tableEditor().draft.at(index)));
    }
    _Projects.tableDefinition().setOpenSides(sides);
}

void TableDefinitionEditorView::paintEvent(QPaintEvent *event)
{
    if (!_Projects.currentProject()) return;
    QWidget::paintEvent(event);
    QPainter painter(this);

    if (_Projects.tableEditor().gridVisible) {
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(QColor(205, 205, 205, 150), 1));
        const qreal scaledGridSpacing = gridSpacing * _Projects.tableEditor().viewScale;
        const QPointF origin = _Projects.currentProject()->gridOrigin;
        qreal firstX = std::fmod(origin.x(), scaledGridSpacing);
        qreal firstY = std::fmod(origin.y(), scaledGridSpacing);
        if (firstX < 0) firstX += scaledGridSpacing;
        if (firstY < 0) firstY += scaledGridSpacing;
        for (qreal x = firstX; x <= width(); x += scaledGridSpacing) {
            painter.drawLine(QPointF(x, 0.0), QPointF(x, height()));
        }
        for (qreal y = firstY; y <= height(); y += scaledGridSpacing) {
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

    if (!_Projects.tableDefinition().isEmpty()) {
        painter.setPen(QPen(QColor(80, 55, 30), 3));
        painter.setBrush(QColor(181, 143, 92, 120));
        const QPainterPath surface = _Projects.tableDefinition().usableArea();
        painter.drawPath(surface);
        if (_Projects.tableEditor().gridVisible) {
            LegoGrid::paintStuds(painter, surface, rect(),
                                 _Projects.currentProject()->gridOrigin,
                                 _Projects.tableEditor().viewScale);
        }

        painter.setPen(editingMeasurementColor);
        for (const TableSurface &surface : _Projects.tableDefinition().surfaces()) {
            const QPolygonF outline = surface.outline();
            for (qsizetype index = 0; index < outline.size(); ++index) {
                const QLineF side(outline.at(index),
                                  outline.at((index + 1) % outline.size()));
                painter.drawText(
                    measurementTextPosition(side),
                    formattedLength(
                        side.length() / (pixelsPerInch * _Projects.tableEditor().viewScale)));
            }
        }


        if (_Projects.tableEditor().selectedSurface >= 0 && _Projects.tableEditor().selectedSide >= 0) {
            const QPolygonF outline =
                _Projects.tableDefinition().surfaces().at(_Projects.tableEditor().selectedSurface).outline();
            painter.setPen(QPen(QColor(255, 80, 70), 6));
            painter.setBrush(Qt::NoBrush);
            painter.drawLine(outline.at(_Projects.tableEditor().selectedSide),
                             outline.at((_Projects.tableEditor().selectedSide + 1) % outline.size()));
        }
    }

    if (!_Projects.tableEditor().active) {
        return;
    }

    if (_Projects.tableEditor().editingIndividualSides) {
        painter.setBrush(editingLineColor);
        for (qsizetype index = 0; index < _Projects.tableEditor().editSides.size(); ++index) {
            const QLineF &side = _Projects.tableEditor().editSides.at(index);
            painter.setPen(index == _Projects.tableEditor().selectedSide
                               ? QPen(QColor(255, 80, 70), 6)
                               : QPen(editingLineColor, 2));
            painter.drawLine(side);
            painter.drawEllipse(side.p1(), 5, 5);
            painter.drawEllipse(side.p2(), 5, 5);
            painter.setPen(editingMeasurementColor);
            painter.drawText(
                measurementTextPosition(side),
                formattedLength(
                    side.length() / (pixelsPerInch * _Projects.tableEditor().viewScale)));
        }
        if (_Projects.tableEditor().drawingSide) {
            painter.setPen(QPen(editingPreviewColor, 1, Qt::DashLine));
            painter.drawLine(_Projects.tableEditor().sideStart, _Projects.tableEditor().cursor);
            paintLiveMeasurement(painter, QLineF(_Projects.tableEditor().sideStart,
                                                  _Projects.tableEditor().cursor));
        }
        return;
    }

    painter.setPen(QPen(editingLineColor, 2));
    painter.setBrush(editingLineColor);
    if (_Projects.tableEditor().draft.size() > 1) {
        painter.drawPolyline(_Projects.tableEditor().draft);
        painter.setPen(editingMeasurementColor);
        for (qsizetype index = 1; index < _Projects.tableEditor().draft.size(); ++index) {
            const QLineF side(_Projects.tableEditor().draft.at(index - 1), _Projects.tableEditor().draft.at(index));
            painter.drawText(
                measurementTextPosition(side),
                formattedLength(
                    side.length() / (pixelsPerInch * _Projects.tableEditor().viewScale)));
        }
        painter.setPen(QPen(editingLineColor, 2));
    }
    for (const QPointF &point : _Projects.tableEditor().draft) {
        painter.drawEllipse(point, 5, 5);
    }
    if (!_Projects.tableEditor().draft.isEmpty()) {
        painter.setPen(QPen(editingPreviewColor, 1, Qt::DashLine));
        painter.drawLine(_Projects.tableEditor().draft.last(), _Projects.tableEditor().cursor);
        if (_Projects.tableEditor().draft.size() >= 3) {
            painter.drawEllipse(_Projects.tableEditor().draft.first(), closePointDistance,
                                closePointDistance);
        }
        if (_Projects.tableEditor().drawingSide) {
            paintLiveMeasurement(painter, QLineF(_Projects.tableEditor().draft.last(),
                                                  _Projects.tableEditor().cursor));
        }
    }
}

void TableDefinitionEditorView::paintLiveMeasurement(QPainter &painter,
                                                    const QLineF &side) const
{
    const QString text = formattedLength(
        side.length() / (pixelsPerInch * _Projects.tableEditor().viewScale));
    const QFontMetricsF metrics(painter.font());
    QRectF label(0, 0, metrics.horizontalAdvance(text) + 12, metrics.height() + 6);

    // Offset the whole label from the line, including for vertical and diagonal sides.
    const QPointF normal = qFuzzyIsNull(side.length())
                               ? QPointF(0, -1)
                               : (measurementTextPosition(side) - side.center()) / 12.0;
    const qreal clearance = qAbs(normal.x()) * label.width() / 2
                            + qAbs(normal.y()) * label.height() / 2 + 8;
    label.moveCenter(side.center() + normal * clearance);
    label.moveLeft(qBound(4.0, label.left(), qMax(4.0, width() - label.width() - 4)));
    label.moveTop(qBound(4.0, label.top(), qMax(4.0, height() - label.height() - 4)));

    painter.save();
    painter.setPen(QPen(palette().color(QPalette::Mid), 1));
    painter.setBrush(palette().color(QPalette::Base));
    painter.drawRoundedRect(label, 4, 4);
    painter.setPen(palette().color(QPalette::Text));
    painter.drawText(label, Qt::AlignCenter, text);
    painter.restore();
}

void TableDefinitionEditorView::panBy(const QPointF &offset)
{
    _Projects.tableDefinition().translate(offset);
    auto &editor = _Projects.tableEditor();
    editor.draft.translate(offset);
    for (QLineF &side : editor.editSides) side.translate(offset);
    editor.sideStart += offset;
    editor.cursor += offset;
    for (CityLayoutElement *plate : _Projects.cityLayouts()) {
        plate->move((QPointF(plate->pos()) + offset).toPoint());
    }
    _Projects.currentProject()->gridOrigin += offset;
    update();
}

void TableDefinitionEditorView::stopPanning()
{
    m_panButton = Qt::NoButton;
    if (m_spaceHeld) setCursor(Qt::OpenHandCursor);
    else if (_Projects.currentProject() && _Projects.tableEditor().active) setCursor(Qt::CrossCursor);
    else unsetCursor();
}

void TableDefinitionEditorView::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spaceHeld = false;
        if (m_panButton == Qt::NoButton) stopPanning();
        event->accept();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void TableDefinitionEditorView::focusOutEvent(QFocusEvent *event)
{
    m_spaceHeld = false;
    stopPanning();
    QWidget::focusOutEvent(event);
}

void TableDefinitionEditorView::wheelEvent(QWheelEvent *event)
{
    if (!_Projects.currentProject()) { event->ignore(); return; }
    const int wheelDelta = event->angleDelta().y();
    if (wheelDelta == 0) {
        QWidget::wheelEvent(event);
        return;
    }
    _Projects.setViewZoom(event->position(),
                         _Projects.currentProject()->zoomFactor * std::pow(1.0015, wheelDelta));
    update();
    event->accept();
}
