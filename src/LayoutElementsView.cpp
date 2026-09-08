#include "LayoutElementsView.h"

#include "LayoutElementWidget.h"
#include "LayoutElementsList.h"

#include <QVBoxLayout>

LayoutElementsView::LayoutElementsView(QWidget *parent)
    : QWidget(parent)
{
}

void LayoutElementsView::addLayoutElements()
{
    setMinimumWidth(150);

    auto *layout = new QVBoxLayout(this);
    const LayoutElementsList elementsList(this);

    for (LayoutElementWidget *element : elementsList.elements()) {
        layout->addWidget(element, 0, Qt::AlignHCenter);
    }

    layout->addStretch();
}
