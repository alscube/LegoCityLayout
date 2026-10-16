
// Copyright 2026. Alan Krzywicki

// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "LayoutElementsView.h"

#include "LayoutElement.h"

#include "ElementCatalog.h"

// #include "StraightTrack.h"
// #include "CurvedTrack.h"
// #include "LeftSwitchTrack.h"
// #include "RightSwitchTrack.h"
// #include "FlexTrack.h"

// #include "StraightRoadPlate.h"
// #include "RoadTurnPlate.h"
// #include "RoadIntersectionPlate.h"
// #include "CrosswalkPlate.h"

#include <QTabWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMap>


LayoutElementsView::LayoutElementsView(QWidget *parent)
    : QWidget(parent)
{
}


struct NewTab LayoutElementsView::FindOrCreateTab( const QString& name )
{
    if (_knownElementTabs.contains(name))
        return _knownElementTabs.value(name);

    auto *page = new QWidget( _elementTab );
    auto *pageLayout = new QVBoxLayout(page);

    auto *scroll = new QScrollArea(_elementTab);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(page);

    _elementTab->addTab(scroll, name);
    const NewTab result{page, pageLayout};
    _knownElementTabs.insert(name, result);

    return result;
}


void LayoutElementsView::addLayoutElements()
{
    if (layout()) return;

    setMinimumWidth(170);

    auto *layout = new QVBoxLayout(this);
    _elementTab = new QTabWidget(this);
     _elementTab->setObjectName(QStringLiteral("layoutElementsTabs"));
    layout->addWidget( _elementTab);

    // Generic way of loading Elements from a directory
    QList<ElementDefinition>& elements = ElementCatalog::instance().loadAllElements();
    for (const auto& element : elements)
    {
        if (element.tabName.isEmpty())
            continue;

        const auto tab = FindOrCreateTab(element.tabName);
        tab.layout->addWidget(new LayoutElement(element, tab.widget), 0, Qt::AlignHCenter);
    }

    // ElementDefinition& elementdef = internalElementsList[0];
    // const auto trackTab = FindOrCreateTab(tr("Tracks"));
    // trackTab.layout->addWidget(new StraightTrack(trackTab.widget), 0, Qt::AlignHCenter);
    // trackTab.layout->addWidget(new CurvedTrack(trackTab.widget), 0, Qt::AlignHCenter);
    // trackTab.layout->addWidget(new LeftSwitchTrack(trackTab.widget), 0, Qt::AlignHCenter);
    // trackTab.layout->addWidget(new RightSwitchTrack(trackTab.widget), 0, Qt::AlignHCenter);
    // trackTab.layout->addWidget(new FlexTrack(trackTab.widget), 0, Qt::AlignHCenter);

    // const auto roadsTab = FindOrCreateTab(tr("Roads"));
    // roadsTab.layout->addWidget(new StraightRoadPlate(roadsTab.widget), 0, Qt::AlignHCenter);
    // roadsTab.layout->addWidget(new RoadTurnPlate(roadsTab.widget), 0, Qt::AlignHCenter);
    // roadsTab.layout->addWidget(new RoadIntersectionPlate(roadsTab.widget), 0, Qt::AlignHCenter);
    // roadsTab.layout->addWidget(new CrosswalkPlate(roadsTab.widget), 0, Qt::AlignHCenter);

    for (const auto& tab : _knownElementTabs)
        tab.layout->addStretch();
}
