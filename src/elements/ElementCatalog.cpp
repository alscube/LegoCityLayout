
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


#include "ElementCatalog.h"
#include "LegoGrid.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QTextStream>
#include <cmath>



ElementCatalog &ElementCatalog::instance()
{
    static ElementCatalog catalog;
    return catalog;
}

ElementCatalog::ElementCatalog()
{
    // Track info is added here becuase it is not loaded from external files

    _allLoadedElements.append({
        {QStringLiteral("Straight Track"), QStringLiteral(":/images/StrightTrack.png"), QSize(16,8)},
        {QStringLiteral("Curved Track (22.5°)"), QStringLiteral(":/images/CurvedTrack.png"), QSize(17,11)},
        {QStringLiteral("Left Switch"), QStringLiteral(":/images/LeftSwitchTrack.png"),
            QSize(qRound(LegoGrid::switchCanvasWidthStuds),qRound(LegoGrid::switchCanvasHeightStuds))},
        {QStringLiteral("Right Switch"), QStringLiteral(":/images/RightSwitchTrack.png"),
            QSize(qRound(LegoGrid::switchCanvasWidthStuds),qRound(LegoGrid::switchCanvasHeightStuds))},
        {QStringLiteral("Flex Track"), QStringLiteral(":/images/FlexTrack.png"), QSize(8,10)}
    });
}

void ElementCatalog::warning( const QString &message ) const
{
    qWarning().noquote() << "Element loading warning:" << message;
}


QString ElementCatalog::plateDirectory() const
{
    const QStringList candidates{
#ifdef LEGO_PLATES_DIRECTORY
        QString::fromUtf8(LEGO_PLATES_DIRECTORY),
#endif
        QDir(QCoreApplication::applicationDirPath()).filePath("../Resources/Elements/Plates"),
        QDir(QCoreApplication::applicationDirPath()).filePath("Elements/Plates"),
        QDir::current().filePath("Elements/Plates")};

    for (const auto &path : candidates)
        if (QDir(path).exists()) return QDir(path).absolutePath();

    return {};
}

QList<ElementDefinition> ElementCatalog::loadPlates()
{
    _plateElements = loadElements(plateDirectory());
    _allLoadedElements.append( _plateElements );
    return _plateElements;
}


QString ElementCatalog::buildingsDirectory() const
{
    const QStringList candidates
    {
#ifdef LEGO_BUILDINGS_DIRECTORY
        QString::fromUtf8(LEGO_BUILDINGS_DIRECTORY),
#endif
        QDir(QCoreApplication::applicationDirPath()).filePath("../Resources/Elements/Buildings"),
        QDir(QCoreApplication::applicationDirPath()).filePath("Elements/Buildings"),
        QDir::current().filePath("Elements/Buildings")
    };

    for (const auto &path : candidates)
        if (QDir(path).exists()) return QDir(path).absolutePath();

    return {};
}

QList<ElementDefinition> ElementCatalog::loadBuildings()
{
    _buildingsElements = loadElements(buildingsDirectory());
    _allLoadedElements.append( _buildingsElements );
    return _buildingsElements;
}




QList<ElementDefinition> ElementCatalog::loadElements(const QString &directory) const
{
    QList<ElementDefinition> loadedElements;

    if (directory.isEmpty() || !QDir(directory).exists()) {
        warning("Directory not found: " + directory);
        return loadedElements;
    }

    const QDir folder(directory);
    for (const auto &info : folder.entryInfoList({"*.info"}, QDir::Files, QDir::Name))
    {
        const auto reject = [&](const QString &reason) {
            warning("Skipping element " + info.fileName() + " : " + reason);
        };

        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            reject(file.errorString());
            continue;
        }

        QTextStream input(&file);
        QStringList lines;
        while (!input.atEnd()) lines.append(input.readLine().trimmed());
        while (!lines.isEmpty() && lines.last().isEmpty()) lines.removeLast();
        if (lines.size() != 4 || lines[0].isEmpty() || lines[2].isEmpty())
        {
            reject("expected tab, width,height, label, and preview scale on four lines");
            continue;
        }

        const auto dimensions = lines[1].split(',');
        bool widthOk = false;
        bool heightOk = false;
        bool scaleOk = false;
        const int width = dimensions.value(0).trimmed().toInt(&widthOk);
        const int height = dimensions.value(1).trimmed().toInt(&heightOk);
        const qreal scale = lines[3].toDouble(&scaleOk);
        if (dimensions.size() != 2 || !widthOk || !heightOk || width <= 0 || height <= 0
            || !scaleOk || !std::isfinite(scale) || scale < 0)
        {
            reject("Invalid stud dimensions or preview scale");
            continue;
        }

        const QString imagePath = folder.absoluteFilePath(info.completeBaseName() + ".png");
        QImageReader image(imagePath);
        if (!image.canRead())
        {
            reject("Matching PNG is missing or unreadable");
            continue;
        }

        const QString type = info.completeBaseName();
        bool duplicate = false;
        for (const auto &plate : loadedElements)
            duplicate |= plate.name == type;
        if (duplicate)
        {
            reject("Duplicate element type: " + type);
            continue;
        }

        loadedElements.append(ElementDefinition{type, imagePath, QSize(width, height), lines[0], lines[2], scale});
    }
    return loadedElements;
}


QList<ElementDefinition> ElementCatalog::loadAllElements()
{
    _plateElements = loadElements(plateDirectory());
    _allLoadedElements.append(_plateElements);

    _buildingsElements = loadElements(buildingsDirectory());
    _allLoadedElements.append(_buildingsElements);

    return _allLoadedElements;
}


const QList<ElementDefinition> &ElementCatalog::allLoadedElements() const
{
    return _allLoadedElements;
}


const ElementDefinition *ElementCatalog::byName(const QString &name) const
{
    for (const auto &element : _allLoadedElements)
        if (element.name == name) return &element;
    // Palette labels originate in .info files and may be saved by the current UI.
    for (const auto &element : _allLoadedElements)
        if (!element.displayName.isEmpty() && element.displayName == name) return &element;
    return nullptr;
}


const ElementDefinition *ElementCatalog::byResource(const QString &resourcePath) const
{
    for (const auto &element : _allLoadedElements)
        if (element.resourcePath == resourcePath) return &element;
    return nullptr;
}
