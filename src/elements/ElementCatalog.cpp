
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


#define TAB_NAME 0
#define IMAGE_SIZE 1
#define DISPLAY_NAME 2
#define PREVIEW_SCALE 3


ElementCatalog &ElementCatalog::instance()
{
    static ElementCatalog catalog;
    return catalog;
}


ElementCatalog::ElementCatalog()
{
}


void ElementCatalog::InternalElements( )
{
    // Track/Road info is added here becuase it is not loaded from external files

    _allLoadedElements.append({
        {QStringLiteral("Straight Track 53401"), QStringLiteral(":/images/StrightTrack.png"), QSize(16,8), "Tracks", 0.50},
        {QStringLiteral("Curved Track 53400 (22.5°)"), QStringLiteral(":/images/CurvedTrack.png"), QSize(17,11), "Tracks", 0.50},
        {QStringLiteral("Left Switch 53407"), QStringLiteral(":/images/LeftSwitchTrack.png"),
         QSize(qRound(LegoGrid::switchCanvasWidthStuds),qRound(LegoGrid::switchCanvasHeightStuds)), "Tracks", 0.50},
        {QStringLiteral("Right Switch 53404"), QStringLiteral(":/images/RightSwitchTrack.png"),
         QSize(qRound(LegoGrid::switchCanvasWidthStuds),qRound(LegoGrid::switchCanvasHeightStuds)), "Tracks", 0.50},
        {QStringLiteral("Flex Track 64022"), QStringLiteral(":/images/FlexTrack.png"), QSize(8,10), "Tracks", 0.50},

        {QStringLiteral("Straight Road 60304"), QStringLiteral(":/images/RoadPlate16x16.png"), QSize(16,16), "Roads", 0.50},
        {QStringLiteral("Road Turn 60304"), QStringLiteral(":/images/RoadTurnPlate16x16.png"), QSize(16,16), "Roads", 0.50},
        {QStringLiteral("Road Intersection 60304"), QStringLiteral(":/images/RoadIntersectionPlate16x16.png"), QSize(16,16), "Roads", 0.50},
        {QStringLiteral("Crosswalk 60304"), QStringLiteral(":/images/CrosswalkPlate8x16.png"), QSize(16,8), "Roads", 0.50}
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
        QStringList infoData;
        while (!input.atEnd()) infoData.append(input.readLine().trimmed());
        while (!infoData.isEmpty() && infoData.last().isEmpty()) infoData.removeLast();
        if (infoData.size() != 4 || infoData[TAB_NAME].isEmpty() || infoData[DISPLAY_NAME].isEmpty())
        {
            reject("expected tab, width,height, label, and preview scale on four lines");
            continue;
        }

        const auto dimensions = infoData[IMAGE_SIZE].split(',');
        bool widthOk = false;
        bool heightOk = false;
        bool scaleOk = false;
        const int width = dimensions.value(0).trimmed().toInt(&widthOk);
        const int height = dimensions.value(1).trimmed().toInt(&heightOk);
        const qreal scale = infoData[PREVIEW_SCALE].toDouble(&scaleOk);
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

        const QString displayName = infoData[DISPLAY_NAME];
        bool duplicate = false;
        for (const auto &element : loadedElements)
            duplicate |= element.displayName == displayName;

        if (duplicate)
        {
            reject("Duplicate element type: " + displayName);
            continue;
        }

        loadedElements.append(ElementDefinition{displayName, imagePath, QSize(width, height), infoData[TAB_NAME], scale});
    }
    return loadedElements;
}


QList<ElementDefinition>& ElementCatalog::loadAllElements()
{
    _plateElements = loadElements(plateDirectory());
    _allLoadedElements.append(_plateElements);

    _buildingsElements = loadElements(buildingsDirectory());
    _allLoadedElements.append(_buildingsElements);

    InternalElements();

    return _allLoadedElements;
}


const QList<ElementDefinition>& ElementCatalog::allLoadedElements() const
{
    return _allLoadedElements;
}


const ElementDefinition *ElementCatalog::byName(const QString &name) const
{
     for (const auto &element : _allLoadedElements)
        if (element.displayName == name)
             return &element;

    // // Palette labels originate in .info files and may be saved by the current UI.
    // for (const auto &element : _allLoadedElements)
    // {
    //     if (!element.displayName.isEmpty() && element.displayName == name)
    //         return &element;
    // }

    return nullptr;
}


const ElementDefinition *ElementCatalog::byResource(const QString &resourcePath) const
{
    for (const auto &element : _allLoadedElements)
        if (element.resourcePath == resourcePath) return &element;
    return nullptr;
}
