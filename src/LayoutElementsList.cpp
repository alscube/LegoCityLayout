#include "LayoutElementsList.h"

#include "LayoutElement.h"

#include <QCoreApplication>

LayoutElementsList::LayoutElementsList(QWidget *parent)
{
    m_elements.append(new LayoutElement(
        QCoreApplication::translate("LayoutElementsView", "16x32"),
        QStringLiteral(":/images/BasePlate16x32Green.png"), parent));
    m_elements.append(new LayoutElement(
        QCoreApplication::translate("LayoutElementsView", "32x32"),
        QStringLiteral(":/images/BasePlate32x32Green.png"), parent));
    m_elements.append(new LayoutElement(
        QCoreApplication::translate("LayoutElementsView", "48x48"),
        QStringLiteral(":/images/BasePlate48x48Gray.png"), parent));
    m_elements.append(new LayoutElement(
        QCoreApplication::translate("LayoutElementsView", "Test 32"),
        QStringLiteral(":/images/TestPlate.png"), parent));
}

const QList<LayoutElement *> &LayoutElementsList::elements() const
{
    return m_elements;
}
