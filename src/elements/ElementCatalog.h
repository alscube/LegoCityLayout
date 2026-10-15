
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

#pragma once

#include <QString>
#include <QSize>
#include <QList>

struct ElementDefinition {
    QString name;
    QString resourcePath;
    QSize size;
    QString tabName;
    QString displayName;
    qreal previewScale = 0.0;
};

// SINGLETON
// Stable, untranslated type names identify the app's current element definitions.
class ElementCatalog final
{
public:
    static ElementCatalog &instance();

    ElementCatalog(const ElementCatalog &) = delete;
    ElementCatalog &operator=(const ElementCatalog &) = delete;
    ElementCatalog(ElementCatalog &&) = delete;
    ElementCatalog &operator=(ElementCatalog &&) = delete;

    QString plateDirectory() const;
    QList<ElementDefinition> loadPlates();

    QString buildingsDirectory() const;
    QList<ElementDefinition> loadBuildings();

    QList<ElementDefinition> loadElements(const QString &directory) const;

    QList<ElementDefinition> loadAllElements();


    const QList<ElementDefinition>& allLoadedElements() const;

    const ElementDefinition *byName(const QString &name) const;
    const ElementDefinition *byResource(const QString &resourcePath) const;

private:
    ElementCatalog();
    void warning(const QString &message) const;

    QList<ElementDefinition> _plateElements;
    QList<ElementDefinition> _buildingsElements;
    QList<ElementDefinition> _allLoadedElements;
};
