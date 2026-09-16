#pragma once

#include <QList>

class CityLayoutElement;

class CityLayoutElements final : public QList<CityLayoutElement *>
{
public:
    using QList<CityLayoutElement *>::QList;

    CityLayoutElement *hoveredElement() const;
    void setHoveredElement(CityLayoutElement *element);
    CityLayoutElement *selectedElement() const;
    void setSelectedElement(CityLayoutElement *element);
    void deleteImage(CityLayoutElement *image);

private:
    CityLayoutElement *_HoveredElement = nullptr;
    CityLayoutElement *_SelectedElement = nullptr;
};
