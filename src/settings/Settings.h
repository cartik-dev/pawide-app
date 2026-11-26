#pragma once
#include <QObject>
#include <QString>

namespace paw {

// Typed access to user preferences stored in the SQLite database.
class Settings : public QObject {
    Q_OBJECT
public:
    static Settings &instance();

    bool showHiddenFiles() const;
    void setShowHiddenFiles(bool v);
    bool confirmDelete() const;
    void setConfirmDelete(bool v);
    int tabSize() const;
    void setTabSize(int v);
    bool insertSpaces() const;
    void setInsertSpaces(bool v);
    bool wordWrap() const;
    void setWordWrap(bool v);
    int fontSize() const;
    void setFontSize(int v);
    QString fontFamily() const;
    void setFontFamily(const QString &v);
    bool autoSave() const;
    void setAutoSave(bool v);

signals:
    void changed(const QString &key);

private:
    Settings();
    bool getBool(const char *key, bool def) const;
    int getInt(const char *key, int def) const;
};

} // namespace paw
