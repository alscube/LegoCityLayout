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
