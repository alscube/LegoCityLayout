#pragma once

#include <QByteArray>
#include <QSettings>

class Preferences final
{
public:
    static Preferences &instance();

    Preferences(const Preferences &) = delete;
    Preferences &operator=(const Preferences &) = delete;
    Preferences(Preferences &&) = delete;
    Preferences &operator=(Preferences &&) = delete;

    QByteArray splitterState() const;
    void setSplitterState(const QByteArray &state);

private:
    Preferences();

    QSettings m_settings;
};
