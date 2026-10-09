
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

#include <QByteArray>
#include <QSettings>

class UserSettings final
{
public:
    enum class MeasurementSystem {
        Imperial,
        Metric,
        Studs,
        Plates
    };

    static UserSettings &instance();

    UserSettings(const UserSettings &) = delete;
    UserSettings &operator=(const UserSettings &) = delete;
    UserSettings(UserSettings &&) = delete;
    UserSettings &operator=(UserSettings &&) = delete;

    QByteArray splitterState() const;
    void setSplitterState(const QByteArray &state);

    QByteArray mainWindowGeometry() const;
    void setMainWindowGeometry(const QByteArray &geometry);

    MeasurementSystem measurementSystem() const;
    void setMeasurementSystem(MeasurementSystem system);
    QString measurementAbbreviation() const;
    QString measurementName() const;
    double measurementUnitsPerInch() const;

private:
    UserSettings();

    QSettings m_settings;
};
