#pragma once
#include <QList>
#include <QObject>
#include <QString>

class QTranslator;

namespace paw {

// Phase-1 plugin runtime: scans the plugins directory and activates the
// plugin types the IDE understands today. Language packs (a compiled Qt
// translation, .qm) are installed as QTranslators before any window is built.
struct PluginInfo {
    QString id;
    QString name;
    QString version;
    QString type;   // "language-pack" | "unknown"
    QString author;
    QString entry;  // type-specific payload file, relative to `path`
    QString path;   // plugin folder
};

class PluginManager {
public:
    static PluginManager &instance();

    // Scans <AppData>/PawIDE/plugins (and <appDir>/plugins for portable use).
    void scan();
    // Installs translators for every discovered language pack. Must run
    // before the first window is created so tr() picks the translations up.
    void activate();

    const QList<PluginInfo> &plugins() const { return m_plugins; }
    QString pluginsDir() const { return m_dir; }

private:
    PluginManager() = default;
    QString m_dir;
    QList<PluginInfo> m_plugins;
    QList<QTranslator *> m_translators;
};

} // namespace paw
