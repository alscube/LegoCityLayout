#include "Preferences.h"

namespace {
constexpr auto splitterStateKey = "mainWindow/splitterState";
}

Preferences &Preferences::instance()
{
    static Preferences preferences;
    return preferences;
}

Preferences::Preferences()
    : m_settings(QStringLiteral("LegoCityLayout"),
                 QStringLiteral("LegoCityLayout"))
{
}

QByteArray Preferences::splitterState() const
{
    return m_settings.value(splitterStateKey).toByteArray();
}

void Preferences::setSplitterState(const QByteArray &state)
{
    m_settings.setValue(splitterStateKey, state);
}
