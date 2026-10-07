#include "LayoutElementsList.h"

#include "LayoutElement.h"

#include <QCoreApplication>

const LayoutElementsList::PlateDefinition LayoutElementsList::standardPlates[] = {
    {"16x32", ":/images/BasePlate16x32Green.png", QSize(16, 32)},
    {"32x32", ":/images/BasePlate32x32Green.png", QSize(32, 32)},
    {"48x48", ":/images/BasePlate48x48Gray.png", QSize(48, 48)},
    {"Test 32", ":/images/TestPlate.png", QSize(32, 32)},
};

LayoutElementsList::LayoutElementsList(QWidget *parent)
{
    for (const PlateDefinition &plate : standardPlates)
    {
        m_elements.append(new LayoutElement(
            QCoreApplication::translate("LayoutElementsView", plate.name),
            QString::fromLatin1(plate.resourcePath), plate.sizeInStuds, parent));
    }
}

const QList<LayoutElement *> &LayoutElementsList::elements() const
{
    return m_elements;
}
