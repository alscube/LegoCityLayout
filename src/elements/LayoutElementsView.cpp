
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

#include "BasePlate16x32Green.h"
#include "BasePlate32x32Green.h"
#include "BasePlate48x48Gray.h"
#include "TestPlate.h"

#include "StraightTrack.h"
#include "CurvedTrack.h"
#include "LeftSwitchTrack.h"
#include "RightSwitchTrack.h"
#include "FlexTrack.h"

#include <QTabWidget>
#include <QScrollArea>
#include <QVBoxLayout>


LayoutElementsView::LayoutElementsView(QWidget *parent)
    : QWidget(parent)
{
}


void LayoutElementsView::addLayoutElements()
{
    if (layout()) return;

    setMinimumWidth(170);

    auto *layout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget(this);
    tabs->setObjectName(QStringLiteral("layoutElementsTabs"));
    layout->addWidget(tabs);

    auto *platesPage = new QWidget(tabs);
    auto *platesLayout = new QVBoxLayout(platesPage);
    tabs->addTab(platesPage, tr("Plates"));
    platesLayout->addWidget(new BasePlate16x32Green(platesPage),0, Qt::AlignHCenter);
    platesLayout->addWidget(new BasePlate32x32Green(platesPage),0, Qt::AlignHCenter);
    platesLayout->addWidget(new BasePlate48x48Gray(platesPage),0, Qt::AlignHCenter);
    platesLayout->addWidget(new TestPlate(platesPage),0, Qt::AlignHCenter);
    platesLayout->addStretch();

    auto *buildingsPage = new QWidget(tabs);
    auto *buildingsLayout = new QVBoxLayout(buildingsPage);
    buildingsLayout->addWidget(new LayoutElement(
        tr("Town Hall"), QStringLiteral(":/images/TownHall.png"),
        QSize(32, 32), buildingsPage), 0, Qt::AlignHCenter);
    buildingsLayout->addStretch();
    tabs->addTab(buildingsPage, tr("Buildings"));

    auto *tracksPage = new QWidget(tabs);
    auto *tracksLayout = new QVBoxLayout(tracksPage);
    tracksLayout->addWidget(new StraightTrack(tracksPage), 0, Qt::AlignHCenter);
    tracksLayout->addWidget(new CurvedTrack(tracksPage), 0, Qt::AlignHCenter);
    tracksLayout->addWidget(new LeftSwitchTrack(tracksPage), 0, Qt::AlignHCenter);
    tracksLayout->addWidget(new RightSwitchTrack(tracksPage), 0, Qt::AlignHCenter);
    tracksLayout->addWidget(new FlexTrack(tracksPage), 0, Qt::AlignHCenter);
    tracksLayout->addStretch();
    auto *tracksScroll = new QScrollArea(tabs);
    tracksScroll->setWidgetResizable(true);
    tracksScroll->setFrameShape(QFrame::NoFrame);
    tracksScroll->setWidget(tracksPage);
    tabs->addTab(tracksScroll, tr("Tracks"));
}
