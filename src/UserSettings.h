#pragma once

#include <QByteArray>
#include <QSettings>

class UserSettings final
{
public:
    enum class MeasurementSystem {
        Imperial,
        Metric
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

private:
    UserSettings();

    QSettings m_settings;
};
