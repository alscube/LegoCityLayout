
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

#include "CityLayoutElement.h"
#include "CityLayoutTrack.h"

#include <QTransform>
#include <QPixmapCache>
#include <cmath>

CityLayoutElement::CityLayoutElement(const QString &name,
                                     const QPixmap &pixmap,
                                     const QSize &plateSize,
                                     qreal pixelsPerStud,
                                     qreal zoomFactor,
                                     QWidget *parent,
                                     const QString &resourcePath)
    : QLabel(parent), m_name(name), m_resourcePath(resourcePath), m_originalPixmap(pixmap)
    , m_plateSize(plateSize)
    , m_unscaledSize(plateSize.width() * pixelsPerStud,
                     plateSize.height() * pixelsPerStud)
{
    m_sourceBounds = m_originalPixmap.rect();
    m_unscaledFootprint = QRectF(QPointF(), m_unscaledSize);
    m_track = CityLayoutTrack::create(resourcePath);
    if (m_track) m_track->calibrate(*this, pixelsPerStud);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setZoomFactor(zoomFactor);
}

CityLayoutElement::~CityLayoutElement() = default;


bool CityLayoutElement::isStraightTrack() const
{
    return m_track && m_track->isStraightTrack();
}

bool CityLayoutElement::isSwitchTrack() const { return m_track && m_track->isSwitchTrack(); }
bool CityLayoutElement::isFlexTrack() const { return m_track && m_track->isFlexTrack(); }
qreal CityLayoutElement::flexBendDegrees() const { return m_track ? m_track->flexBendDegrees() : 0.0; }

QList<QLineF> CityLayoutElement::trackConnections() const
{
    return m_track ? m_track->connections(*this) : QList<QLineF>();
}

QString CityLayoutElement::name() const
{
    return m_name;
}


void CityLayoutElement::setZoomFactor(qreal zoomFactor)
{
    m_zoomFactor = zoomFactor;
    const QSizeF rotatedSize = QTransform().rotate(rotationDegrees())
        .mapRect(QRectF(QPointF(), m_unscaledSize)).size();
    // Tracks keep their physical scale; transparent padding fills the last pixel.
    const QSize size(qMax(1, m_track ? qCeil(rotatedSize.width() * zoomFactor)
                                    : qRound(rotatedSize.width() * zoomFactor)),
                     qMax(1, m_track ? qCeil(rotatedSize.height() * zoomFactor)
                                    : qRound(rotatedSize.height() * zoomFactor)));
    const qreal pixelRatio = devicePixelRatioF();
    const QSize renderSize(qRound(size.width() * pixelRatio), qRound(size.height() * pixelRatio));
    QPixmap displayedPixmap;
    if (m_track) displayedPixmap = m_track->renderArtwork(*this, renderSize);
    else {
        const QString key = QStringLiteral("layout-artwork:%1:%2")
            .arg(m_originalPixmap.cacheKey()).arg(rotationDegrees(), 0, 'g', 17);
        QPixmap artwork;
        if (!QPixmapCache::find(key, &artwork)) {
            artwork = m_originalPixmap.copy(m_sourceBounds).transformed(
                QTransform().rotate(rotationDegrees()), Qt::SmoothTransformation);
            QPixmapCache::insert(key, artwork);
        }
        displayedPixmap = artwork.scaled(renderSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
    displayedPixmap.setDevicePixelRatio(pixelRatio);
    setPixmap(displayedPixmap);
    setFixedSize(size);
    if (m_track) m_track->updateSelectionBounds(displayedPixmap, size);
    update();
}

QPointF CityLayoutElement::layoutPosition() const
{
    return m_hasPrecisePosition && pos() == m_lastZoomPosition ? m_precisePosition : QPointF(pos());
}

void CityLayoutElement::setLayoutPosition(const QPointF &position)
{
    m_precisePosition = position;
    move(position.toPoint());
    m_lastZoomPosition = pos();
    m_hasPrecisePosition = true;
}

void CityLayoutElement::zoomFromAnchor(const QPointF &anchor, qreal relativeScale, qreal zoomFactor)
{
    const QPointF position = anchor + (layoutPosition() - anchor) * relativeScale;
    setZoomFactor(zoomFactor);
    setLayoutPosition(position);
}

bool CityLayoutElement::containsPoint(const QPoint &localPoint) const
{
    return rect().contains(localPoint) && (!m_track || m_track->containsPoint(rect(), localPoint));
}

void CityLayoutElement::setSelectionHighlighted(bool highlighted)
{
    m_selectionHighlighted = highlighted;
    setStyleSheet(highlighted && !m_track
        ? QStringLiteral("border: 2px solid orange;") : QString());
    update();
}

void CityLayoutElement::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    if (m_track) m_track->paintSelection(*this);
}

QRectF CityLayoutElement::footprintRect() const
{
    const QTransform rotation = QTransform().rotate(rotationDegrees());
    const QRectF imageBounds = rotation.mapRect(QRectF(QPointF(), m_unscaledSize));
    QRectF footprint = rotation.mapRect(m_unscaledFootprint);
    footprint.translate(-imageBounds.topLeft());
    const qreal scaleX = m_track ? m_zoomFactor : width() / imageBounds.width();
    const qreal scaleY = m_track ? m_zoomFactor : height() / imageBounds.height();
    return QRectF(footprint.x() * scaleX, footprint.y() * scaleY,
                  footprint.width() * scaleX, footprint.height() * scaleY);
}

QPixmap CityLayoutElement::savedPixmap() const
{
    // Save full source detail in the same orientation as the displayed plate.
    return m_originalPixmap.transformed(
        QTransform().rotate(rotationDegrees()), Qt::SmoothTransformation);
}

void CityLayoutElement::rotateQuarterTurns(int turns)
{
    rotateByDegrees(turns * 90.0);
}

qreal CityLayoutElement::rotationStepDegrees() const
{
    return m_track ? m_track->rotationStepDegrees() : 90.0;
}

void CityLayoutElement::rotateSteps(int steps)
{
    rotateByDegrees(steps * rotationStepDegrees());
}

void CityLayoutElement::rotateByDegrees(qreal degrees)
{
    const QPointF center = layoutPosition() + footprintRect().center();
    m_rotationDegrees = std::fmod(m_rotationDegrees + degrees, 360.0);
    if (m_rotationDegrees < 0.0) m_rotationDegrees += 360.0;
    setZoomFactor(m_zoomFactor);
    const QPointF position = center - footprintRect().center();
    setLayoutPosition(position);
}

bool CityLayoutElement::setFlexBendDegrees(qreal degrees)
{
    if (!m_track || !m_track->setFlexBendDegrees(degrees)) return false;
    setZoomFactor(m_zoomFactor);
    return true;
}
