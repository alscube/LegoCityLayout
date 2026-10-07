#pragma once

#include <QList>
#include <QSize>

class LayoutElement;
class QWidget;

class LayoutElementsList final
{
public:
    explicit LayoutElementsList(QWidget *parent);

    const QList<LayoutElement *> &elements() const;

private:
    struct PlateDefinition {
        const char *name;
        const char *resourcePath;
        QSize sizeInStuds;
    };
    static const PlateDefinition standardPlates[];

    QList<LayoutElement *> m_elements;
};
