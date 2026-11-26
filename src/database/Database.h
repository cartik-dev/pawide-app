#pragma once
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QString>
#include <QVariant>

namespace paw {

// SQLite-backed storage for settings, recent projects and window layout.
class Database : public QObject {
    Q_OBJECT
public:
    struct RecentProject {
        QString name;
        QString path;
        QDateTime lastOpened;
    };

    explicit Database(QObject *parent = nullptr);
    ~Database() override;

    // `file` may be ":memory:" (used by tests).
    bool open(const QString &file);
    bool isOpen() const;
    static QString defaultPath();

    QVariant value(const QString &key, const QVariant &defaultValue = {}) const;
    void setValue(const QString &key, const QVariant &value);

    QList<RecentProject> recentProjects() const;
    void addRecentProject(const QString &path);
    void removeRecentProject(const QString &path);
    void clearRecentProjects();

signals:
    void valueChanged(const QString &key);
    void recentProjectsChanged();

private:
    static QString normalised(const QString &path);
    QString m_connection;
};

} // namespace paw
