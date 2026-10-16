
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


#include "FlexTrack.h"

#include <QCoreApplication>

#define PREVIEW_SCALE 0.75


FlexTrack::FlexTrack(QWidget *parent)
    : LayoutElement(QCoreApplication::translate("LayoutElementsView", "Flex Track 64022"),
                    QStringLiteral(":/images/FlexTrack.png"),
                    QSize(8, 10), parent)
{
    initElementImage();
}



QPixmap FlexTrack::loadElementImage( const QString& resourcePath, qreal previewScale )
{
    QPixmap elementImage = LayoutElement::loadElementImage(resourcePath, PREVIEW_SCALE );

    return elementImage;
}