
#include "LayoutElement.h"
#include "CityLayoutElement.h"

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

QPixmap previewPixmap(const QString &resourcePath, const QSize &plateSize)
{
    const QPixmap source(resourcePath);
    if (plateSize == QSize(17, 11) || plateSize == QSize(8, 16)) {
        CityLayoutElement preview(QString(), source, plateSize, 8.0);
        QPixmap artwork = preview.pixmap();
        artwork.setDevicePixelRatio(1.0);
        return artwork;
    }
    return source;
}
}

LayoutElement::LayoutElement(const QString &name,
                                         const QString &resourcePath,
                                         const QSize &plateSize,
                                         QWidget *parent)
    : QWidget(parent), m_name(name), m_resourcePath(resourcePath), m_plateSize(plateSize)
{
    setCursor(Qt::OpenHandCursor);
    setObjectName(QStringLiteral("layoutElementWidget"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    auto *image = new QLabel(this);
    image->setAttribute(Qt::WA_TransparentForMouseEvents);
    image->setAlignment(Qt::AlignCenter);
    QPixmap elementImage = previewPixmap(resourcePath, plateSize);
//    QSize imageSize = elementImage.size();
    // Keep large source assets from expanding the palette beyond the window.
    if (elementImage.width() > 160 || elementImage.height() > 160) {
        elementImage = elementImage.scaled(
            QSize(160, 160), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    image->setPixmap(elementImage);

    // image->setPixmap(QPixmap(resourcePath).scaled(
    //     QSize(160, 160), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto *label = new QLabel(name, this);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(image);
    layout->addWidget(label);
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

    const QPixmap sourcePixmap = previewPixmap(m_resourcePath, m_plateSize);
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
