
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


#include "StraightRoadPlate.h"

#include <QCoreApplication>

namespace {
constexpr qreal previewScale = 0.50;
}


StraightRoadPlate::StraightRoadPlate(QWidget *parent)
    : LayoutElement(QCoreApplication::translate("LayoutElementsView", "Straight Road 60304"),
                    QStringLiteral(":/images/RoadPlate16x16.png"),
                    QSize(16, 16), parent)
{
    initElementImage();
}


QPixmap StraightRoadPlate::loadElementImage( const QString& resourcePath, qreal )
{
    QPixmap elementImage = LayoutElement::loadElementImage(resourcePath, previewScale);

    return elementImage;
}
