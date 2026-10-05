#include "ProjectData.h"

ProjectData::ProjectData(const QString &title)
    : _ProjectTitle(title)
{
    cityLayouts.initializeCityElements(title);
}


ProjectData::~ProjectData()
{
    cityLayouts.initializeCityElements(QString());
}


QString ProjectData::title()
{
    return _ProjectTitle;
}


void ProjectData::setTitle(const QString &title)
{
    _ProjectTitle = title;
}
