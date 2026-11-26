#include "Settings.h"

#include "core/AppContext.h"
#include "database/Database.h"

#include <QtGlobal>

namespace paw {

Settings &Settings::instance()
{
    static Settings s;
    return s;
}

Settings::Settings()
{
    connect(&appDatabase(), &Database::valueChanged, this, &Settings::changed);
}

bool Settings::getBool(const char *key, bool def) const
{
    return appDatabase().value(QLatin1String(key), def ? QStringLiteral("true") : QStringLiteral("false")).toString()
           == QLatin1String("true");
}

int Settings::getInt(const char *key, int def) const
{
    bool ok = false;
    const int v = appDatabase().value(QLatin1String(key), def).toInt(&ok);
    return ok ? v : def;
}

#define PAW_BOOL(Getter, Setter, key, def)                                                    \
    bool Settings::Getter() const { return getBool(key, def); }                               \
    void Settings::Setter(bool v) { appDatabase().setValue(QLatin1String(key), v ? QStringLiteral("true") : QStringLiteral("false")); }

PAW_BOOL(showHiddenFiles, setShowHiddenFiles, "explorer.showHidden", true)
PAW_BOOL(confirmDelete, setConfirmDelete, "explorer.confirmDelete", true)
PAW_BOOL(insertSpaces, setInsertSpaces, "editor.insertSpaces", true)
PAW_BOOL(wordWrap, setWordWrap, "editor.wordWrap", false)
PAW_BOOL(autoSave, setAutoSave, "editor.autoSave", false)

int Settings::tabSize() const { return qBound(1, getInt("editor.tabSize", 4), 16); }
void Settings::setTabSize(int v) { appDatabase().setValue(QStringLiteral("editor.tabSize"), qBound(1, v, 16)); }
int Settings::fontSize() const { return qBound(6, getInt("editor.fontSize", 11), 48); }
void Settings::setFontSize(int v) { appDatabase().setValue(QStringLiteral("editor.fontSize"), qBound(6, v, 48)); }
QString Settings::fontFamily() const { return appDatabase().value(QStringLiteral("editor.fontFamily"), QString()).toString(); }
void Settings::setFontFamily(const QString &v) { appDatabase().setValue(QStringLiteral("editor.fontFamily"), v); }

} // namespace paw
