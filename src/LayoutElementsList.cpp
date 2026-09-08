#include "LayoutElementsList.h"

#include "LayoutElementWidget.h"

#include <QCoreApplication>

LayoutElementsList::LayoutElementsList(QWidget *parent)
{
    m_elements.append(new LayoutElementWidget(
        QCoreApplication::translate("LayoutElementsView", "16x32"),
        QStringLiteral(":/images/BasePlate16x32Green.png"), parent));
    m_elements.append(new LayoutElementWidget(
        QCoreApplication::translate("LayoutElementsView", "32x32"),
        QStringLiteral(":/images/BasePlate32x32Green.png"), parent));
    m_elements.append(new LayoutElementWidget(
        QCoreApplication::translate("LayoutElementsView", "48x48"),
        QStringLiteral(":/images/BasePlate48x48Gray.png"), parent));
}

const QList<LayoutElementWidget *> &LayoutElementsList::elements() const
{
    return m_elements;
}
