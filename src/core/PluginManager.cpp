#include "PluginManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTranslator>

namespace paw {

PluginManager &PluginManager::instance()
{
    static PluginManager manager;
    return manager;
}

void PluginManager::scan()
{
    m_plugins.clear();
    // PAWIDE_PLUGINS_DIR replaces the default location (tests and screenshots).
    const QByteArray envDir = qgetenv("PAWIDE_PLUGINS_DIR");
    QStringList roots = envDir.isEmpty()
                            ? QStringList {
                                  QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                      + QStringLiteral("/plugins"),
                                  QCoreApplication::applicationDirPath() + QStringLiteral("/plugins")}
                            : QStringList {QString::fromLocal8Bit(envDir)};
    for (const QString &root : std::as_const(roots)) {
        QDir rootDir(root);
        if (!rootDir.exists() && root == roots.first())
            rootDir.mkpath(QStringLiteral("."));
        QDirIterator it(root, QDir::Dirs | QDir::NoDotAndDotDot);
        while (it.hasNext()) {
            const QString dir = it.next();
            QFile manifest(dir + QStringLiteral("/plugin.json"));
            if (!manifest.open(QIODevice::ReadOnly))
                continue;
            const QJsonObject o = QJsonDocument::fromJson(manifest.readAll()).object();
            PluginInfo info;
            info.id = o.value(QStringLiteral("id")).toString();
            info.name = o.value(QStringLiteral("name")).toString();
            info.version = o.value(QStringLiteral("version")).toString();
            info.type = o.value(QStringLiteral("type")).toString();
            info.author = o.value(QStringLiteral("author")).toString();
            info.entry = o.value(QStringLiteral("entry")).toString();
            info.path = dir;
            if (info.id.isEmpty())
                continue;
            m_plugins.push_back(info);
        }
    }
    m_dir = roots.first();
}

void PluginManager::activate()
{
    for (const PluginInfo &p : std::as_const(m_plugins)) {
        if (p.type != QLatin1String("language-pack") || p.entry.isEmpty())
            continue;
        QString entry = p.entry;
        if (!QFileInfo(entry).isAbsolute())
            entry = p.path + QLatin1Char('/') + entry;
        auto *translator = new QTranslator(QCoreApplication::instance());
        if (translator->load(entry) && QCoreApplication::installTranslator(translator))
            m_translators.push_back(translator);
        else
            translator->deleteLater();
    }
}

} // namespace paw
