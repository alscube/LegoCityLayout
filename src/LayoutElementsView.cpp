
#include "LayoutElementsView.h"

#include "LayoutElement.h"
#include "LayoutElementsList.h"

#include <QVBoxLayout>


LayoutElementsView::LayoutElementsView(QWidget *parent)
    : QWidget(parent)
{
}


void LayoutElementsView::addLayoutElements()
{
    setMinimumWidth(170);

    auto *layout = new QVBoxLayout(this);
    const LayoutElementsList elementsList(this);

    for (LayoutElement *element : elementsList.elements()) {
        layout->addWidget(element, 0, Qt::AlignHCenter);
    }

    layout->addStretch();
}
