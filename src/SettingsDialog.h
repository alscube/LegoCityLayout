#pragma once

#include <QDialog>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog final : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = nullptr);
    ~SettingsDialog() override;

private:
    void saveSettings();

    Ui::SettingsDialog *m_ui = nullptr;
};
