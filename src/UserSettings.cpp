#include "UserSettings.h"
#include "LegoGrid.h"

#include <QCoreApplication>

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
    const QString value = m_settings.value(measurementSystemKey).toString();
    if (value == metricValue) return MeasurementSystem::Metric;
    if (value == QStringLiteral("studs")) return MeasurementSystem::Studs;
    if (value == QStringLiteral("plates")) return MeasurementSystem::Plates;
    return MeasurementSystem::Imperial;
}

void UserSettings::setMeasurementSystem(MeasurementSystem system)
{
    QString value;
    switch (system) {
    case MeasurementSystem::Metric: value = QStringLiteral("metric"); break;
    case MeasurementSystem::Studs: value = QStringLiteral("studs"); break;
    case MeasurementSystem::Plates: value = QStringLiteral("plates"); break;
    default: value = QStringLiteral("imperial"); break;
    }
    m_settings.setValue(measurementSystemKey, value);
}

QString UserSettings::measurementAbbreviation() const
{
    switch (measurementSystem()) {
    case MeasurementSystem::Metric: return QStringLiteral("CM");
    case MeasurementSystem::Studs: return QStringLiteral("ST");
    case MeasurementSystem::Plates: return QStringLiteral("PL");
    default: return QStringLiteral("IN");
    }
}

QString UserSettings::measurementName() const
{
    switch (measurementSystem()) {
    case MeasurementSystem::Metric: return QCoreApplication::translate("UserSettings", "Centimeters");
    case MeasurementSystem::Studs: return QCoreApplication::translate("UserSettings", "Studs");
    case MeasurementSystem::Plates: return QCoreApplication::translate("UserSettings", "Plates");
    default: return QCoreApplication::translate("UserSettings", "Inches");
    }
}

double UserSettings::measurementUnitsPerInch() const
{
    switch (measurementSystem()) {
    case MeasurementSystem::Metric: return LegoGrid::millimetersPerInch / 10.0;
    case MeasurementSystem::Studs: return LegoGrid::millimetersPerInch / LegoGrid::studPitchMillimeters;
    case MeasurementSystem::Plates: return LegoGrid::millimetersPerInch / (LegoGrid::studPitchMillimeters * LegoGrid::plateStuds);
    default: return 1.0;
    }
}
