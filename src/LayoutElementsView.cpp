
#include "LayoutElementsView.h"

#include "LayoutElement.h"
#include "LayoutElementsList.h"

#include <QTabWidget>
#include <QVBoxLayout>


LayoutElementsView::LayoutElementsView(QWidget *parent)
    : QWidget(parent)
{
}


void LayoutElementsView::addLayoutElements()
{
    if (layout()) return;

    setMinimumWidth(170);

    auto *layout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget(this);
    tabs->setObjectName(QStringLiteral("layoutElementsTabs"));
    layout->addWidget(tabs);

    auto *platesPage = new QWidget(tabs);
    auto *platesLayout = new QVBoxLayout(platesPage);
    tabs->addTab(platesPage, tr("Plates"));
    auto *buildingsPage = new QWidget(tabs);
    auto *buildingsLayout = new QVBoxLayout(buildingsPage);
    buildingsLayout->addWidget(new LayoutElement(
        tr("Town Hall"), QStringLiteral(":/images/TownHall.png"),
        QSize(32, 32), buildingsPage), 0, Qt::AlignHCenter);
    buildingsLayout->addStretch();
    tabs->addTab(buildingsPage, tr("Buildings"));
    auto *tracksPage = new QWidget(tabs);
    auto *tracksLayout = new QVBoxLayout(tracksPage);
    tracksLayout->addWidget(new LayoutElement(
        tr("Straight Track"), QStringLiteral(":/images/StrightTrack.png"),
        QSize(8, 16), tracksPage), 0, Qt::AlignHCenter);
    tracksLayout->addWidget(new LayoutElement(
        tr("Curved Track (22.5°)"), QStringLiteral(":/images/CurvedTrack.png"),
        QSize(17, 11), tracksPage), 0, Qt::AlignHCenter);
    tracksLayout->addStretch();
    tabs->addTab(tracksPage, tr("Tracks"));

    const LayoutElementsList elementsList(platesPage);

    for (LayoutElement *element : elementsList.elements()) {
        platesLayout->addWidget(element, 0, Qt::AlignHCenter);
    }

    platesLayout->addStretch();
}
