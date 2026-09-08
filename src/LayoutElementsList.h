#pragma once

#include <QList>

class LayoutElementWidget;
class QWidget;

class LayoutElementsList final
{
public:
    explicit LayoutElementsList(QWidget *parent);

    const QList<LayoutElementWidget *> &elements() const;

private:
    QList<LayoutElementWidget *> m_elements;
};
