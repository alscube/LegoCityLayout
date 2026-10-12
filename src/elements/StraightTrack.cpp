
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


#include "StraightTrack.h"

#include <QCoreApplication>


#define PREVIEW_SCALE 0.50


StraightTrack::StraightTrack(QWidget *parent)
    : LayoutElement(QCoreApplication::translate("LayoutElementsView", "Straight Track"),
                    QStringLiteral(":/images/StrightTrack.png"), QSize(8, 16), parent)
{
    initElementImage();
}


QPixmap StraightTrack::loadElementImage( const QString& resourcePath )
{
    QPixmap elementImage = LayoutElement::loadElementImage(resourcePath);

    elementImage = elementImage.scaled(elementImage.size() * PREVIEW_SCALE,
            Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return elementImage;
}

