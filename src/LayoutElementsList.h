#pragma once

#include <QList>

class LayoutElement;
class QWidget;

class LayoutElementsList final
{
public:
    explicit LayoutElementsList(QWidget *parent);

    const QList<LayoutElement *> &elements() const;

private:
    QList<LayoutElement *> m_elements;
};
