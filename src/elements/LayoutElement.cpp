
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

#include "LayoutElement.h"

#include <QApplication>
#include <QDrag>
#include <QLabel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QVBoxLayout>


namespace {
constexpr auto layoutElementMimeType = "application/x-legocity-layout-element";
constexpr auto layoutElementNameMimeType = "application/x-legocity-layout-element-name";
constexpr auto layoutElementSizeMimeType = "application/x-legocity-layout-element-size";
constexpr int dragThumbnailSize = 120;

}

LayoutElement::LayoutElement(const QString &name,
                                         const QString &resourcePath,
                                         const QSize &plateSize,
                                         QWidget *parent)
    : QWidget(parent)
    , m_name(name)
    , m_resourcePath(resourcePath)
    , m_plateSize(plateSize)
{
    // this needs to be called in final constructor so
    // it can be overridden by derived classes for custom
    // image scaling

    // initElementImage();
}


void LayoutElement::initElementImage( )
{
    if (layout()) {
        auto *image = qobject_cast<QLabel *>(layout()->itemAt(0)->widget());
        if (image) image->setPixmap(loadElementImage(m_resourcePath));
        return;
    }

    setCursor(Qt::OpenHandCursor);
    setObjectName(QStringLiteral("layoutElementWidget"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    auto *image = new QLabel(this);
    image->setAttribute(Qt::WA_TransparentForMouseEvents);
    image->setAlignment(Qt::AlignCenter);

    QPixmap elementImage = loadElementImage(m_resourcePath);
    image->setPixmap(elementImage);

    auto *label = new QLabel(m_name, this);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(image);
    layout->addWidget(label);
}


QPixmap LayoutElement::loadElementImage( const QString& resourcePath )
{
    QPixmap elementImage(resourcePath);

    // Keep large source assets from expanding the palette beyond the window.
    if (elementImage.width() > MAX_WIDTH || elementImage.height() > MAX_HEIGHT) {
        elementImage = elementImage.scaled(QSize(MAX_WIDTH, MAX_HEIGHT), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    return elementImage;
}


void LayoutElement::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
    }
    QWidget::mousePressEvent(event);
}

void LayoutElement::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)
        || (event->position().toPoint() - m_dragStartPosition).manhattanLength()
               < QApplication::startDragDistance()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    auto *mimeData = new QMimeData;
    mimeData->setData(layoutElementMimeType, m_resourcePath.toUtf8());
    mimeData->setData(layoutElementNameMimeType, m_name.toUtf8());
    mimeData->setData(layoutElementSizeMimeType, QJsonDocument(QJsonObject{
        {QStringLiteral("widthStuds"), m_plateSize.width()},
        {QStringLiteral("heightStuds"), m_plateSize.height()}}).toJson(QJsonDocument::Compact));

    auto *drag = new QDrag(this);
    drag->setMimeData(mimeData);

    const QPixmap sourcePixmap(m_resourcePath);
    if (!sourcePixmap.isNull()) {
        const QPixmap thumbnail = sourcePixmap.scaled(
            QSize(dragThumbnailSize, dragThumbnailSize),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
        drag->setPixmap(thumbnail);
        drag->setHotSpot(QPoint(thumbnail.width() / 2,
                                thumbnail.height() / 2));
    }

    drag->exec(Qt::CopyAction);
    setCursor(Qt::OpenHandCursor);
}

void LayoutElement::mouseReleaseEvent(QMouseEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    QWidget::mouseReleaseEvent(event);
}
