#include "UserSettings.h"

namespace {
constexpr auto splitterStateKey = "mainWindow/splitterState";
constexpr auto mainWindowGeometryKey = "mainWindow/geometry";
constexpr auto measurementSystemKey = "measurementSystem";
constexpr auto metricValue = "metric";
}

UserSettings &UserSettings::instance()
{
    static UserSettings userSettings;
    return userSettings;
}

UserSettings::UserSettings()
    : m_settings(QStringLiteral("LegoCityLayout"),
                 QStringLiteral("LegoCityLayout"))
{
}

QByteArray UserSettings::splitterState() const
{
    return m_settings.value(splitterStateKey).toByteArray();
}

void UserSettings::setSplitterState(const QByteArray &state)
{
    m_settings.setValue(splitterStateKey, state);
}

QByteArray UserSettings::mainWindowGeometry() const
{
    return m_settings.value(mainWindowGeometryKey).toByteArray();
}

void UserSettings::setMainWindowGeometry(const QByteArray &geometry)
{
    m_settings.setValue(mainWindowGeometryKey, geometry);
}

UserSettings::MeasurementSystem UserSettings::measurementSystem() const
{
    return m_settings.value(measurementSystemKey).toString() == metricValue
               ? MeasurementSystem::Metric
               : MeasurementSystem::Imperial;
}

void UserSettings::setMeasurementSystem(MeasurementSystem system)
{
    m_settings.setValue(measurementSystemKey,
                        system == MeasurementSystem::Metric
                            ? metricValue
                            : "imperial");
}
