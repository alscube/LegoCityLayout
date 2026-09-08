#include "CityLayout.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QLabel>
#include <QMimeData>
#include <QPixmap>

namespace {
constexpr auto layoutElementMimeType = "application/x-legocity-layout-element";
}

CityLayout::CityLayout(QWidget *parent)
    : QWidget(parent)
{
    setMinimumWidth(150);
    setAcceptDrops(true);

    auto *title = new QLabel(tr("City Layout"), this);
    title->setAttribute(Qt::WA_TransparentForMouseEvents);
    title->move(12, 12);
    title->show();
}

void CityLayout::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(layoutElementMimeType)) {
        event->acceptProposedAction();
    }
}

void CityLayout::dropEvent(QDropEvent *event)
{
    const QString resourcePath = QString::fromUtf8(
        event->mimeData()->data(layoutElementMimeType));
    const QPixmap pixmap(resourcePath);
    if (pixmap.isNull()) {
        event->ignore();
        return;
    }

    auto *image = new QLabel(this);
    image->setPixmap(pixmap);
    image->setFixedSize(pixmap.size());

    const QPoint requestedPosition = event->position().toPoint()
                                     - QPoint(pixmap.width() / 2,
                                              pixmap.height() / 2);
    const int x = qBound(0, requestedPosition.x(),
                         qMax(0, width() - pixmap.width()));
    const int y = qBound(0, requestedPosition.y(),
                         qMax(0, height() - pixmap.height()));
    image->move(x, y);
    image->show();

    event->acceptProposedAction();
}
