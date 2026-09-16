#include "CityLayoutElements.h"

#include "CityLayoutElement.h"

CityLayoutElement *CityLayoutElements::hoveredElement() const
{
    return _HoveredElement;
}

void CityLayoutElements::setHoveredElement(CityLayoutElement *element)
{
    _HoveredElement = element;
}

CityLayoutElement *CityLayoutElements::selectedElement() const
{
    return _SelectedElement;
}

void CityLayoutElements::setSelectedElement(CityLayoutElement *element)
{
    _SelectedElement = element;
}

void CityLayoutElements::deleteImage(CityLayoutElement *image)
{
    if (!image) {
        return;
    }

    removeAll(image);
    if (_HoveredElement == image) {
        _HoveredElement = nullptr;
    }
    if (_SelectedElement == image) {
        _SelectedElement = nullptr;
    }
    delete image;
}
