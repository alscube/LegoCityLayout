
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
#include "LayoutElementsList.h"

#include "LayoutElement.h"

#include <QCoreApplication>

const LayoutElementsList::PlateDefinition LayoutElementsList::standardPlates[] = {
    {"16x32", ":/images/BasePlate16x32Green.png", QSize(16, 32)},
    {"32x32", ":/images/BasePlate32x32Green.png", QSize(32, 32)},
    {"48x48", ":/images/BasePlate48x48Gray.png", QSize(48, 48)},
    {"Test 32", ":/images/TestPlate.png", QSize(32, 32)},
};

LayoutElementsList::LayoutElementsList(QWidget *parent)
{
    for (const PlateDefinition &plate : standardPlates)
    {
        m_elements.append(new LayoutElement(
            QCoreApplication::translate("LayoutElementsView", plate.name),
            QString::fromLatin1(plate.resourcePath), plate.sizeInStuds, parent));
    }
}

const QList<LayoutElement *> &LayoutElementsList::elements() const
{
    return m_elements;
}
