
#include "LayoutElementWidget.h"

#include <QApplication>
#include <QDrag>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>
#include <QVBoxLayout>

namespace {
constexpr auto layoutElementMimeType = "application/x-legocity-layout-element";
}

LayoutElementWidget::LayoutElementWidget(const QString &name,
                                         const QString &resourcePath,
                                         QWidget *parent)
    : QWidget(parent), m_resourcePath(resourcePath)
{
    setCursor(Qt::OpenHandCursor);
    setObjectName(QStringLiteral("layoutElementWidget"));

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(4);

    auto *image = new QLabel(this);
    image->setAttribute(Qt::WA_TransparentForMouseEvents);
    image->setAlignment(Qt::AlignCenter);
    image->setPixmap(QPixmap(resourcePath).scaled(
        QSize(160, 160), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    auto *label = new QLabel(name, this);
    label->setAttribute(Qt::WA_TransparentForMouseEvents);
    label->setAlignment(Qt::AlignCenter);

    layout->addWidget(image);
    layout->addWidget(label);
}

void LayoutElementWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
    }
    QWidget::mousePressEvent(event);
}

void LayoutElementWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)
        || (event->position().toPoint() - m_dragStartPosition).manhattanLength()
               < QApplication::startDragDistance()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    auto *mimeData = new QMimeData;
    mimeData->setData(layoutElementMimeType, m_resourcePath.toUtf8());

    auto *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
    setCursor(Qt::OpenHandCursor);
}

void LayoutElementWidget::mouseReleaseEvent(QMouseEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    QWidget::mouseReleaseEvent(event);
}
