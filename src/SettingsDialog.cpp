
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
#include "SettingsDialog.h"

#include "UserSettings.h"
#include "ui_SettingsDialog.h"

#include <QComboBox>

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::SettingsDialog)
{
    m_ui->setupUi(this);

    const int currentSystem = static_cast<int>(
        UserSettings::instance().measurementSystem());
    m_ui->measurementSystemComboBox->setCurrentIndex(currentSystem);

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted,
            this, &SettingsDialog::saveSettings);
    connect(m_ui->buttonBox, &QDialogButtonBox::rejected,
            this, &QDialog::reject);
}

SettingsDialog::~SettingsDialog()
{
    delete m_ui;
}

void SettingsDialog::saveSettings()
{
    const auto system = static_cast<UserSettings::MeasurementSystem>(
        m_ui->measurementSystemComboBox->currentIndex());
    UserSettings::instance().setMeasurementSystem(system);
    accept();
}
