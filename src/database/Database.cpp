#include "Database.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

namespace paw {

static constexpr int kMaxRecent = 20;

Database::Database(QObject *parent) : QObject(parent) {}

Database::~Database()
{
    if (m_connection.isEmpty())
        return;
    {
        QSqlDatabase db = QSqlDatabase::database(m_connection, false);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_connection);
}

QString Database::defaultPath()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return QDir(dir).filePath(QStringLiteral("pawide.db"));
}

bool Database::open(const QString &file)
{
    if (isOpen())
        return true;
    const QString conn = QStringLiteral("pawide-") + QUuid::createUuid().toString(QUuid::Id128);
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(file);
        if (!db.open()) {
            qWarning("Paw IDE: cannot open database: %s", qPrintable(db.lastError().text()));
            m_connection = conn; // so the destructor removes it
            return false;
        }
        QSqlQuery q(db);
        q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS settings (key TEXT PRIMARY KEY, value TEXT)"));
        q.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS recent_projects ("
                              "path TEXT PRIMARY KEY, name TEXT NOT NULL, last_opened INTEGER NOT NULL)"));
    }
    m_connection = conn;
    return true;
}

bool Database::isOpen() const
{
    if (m_connection.isEmpty())
        return false;
    return QSqlDatabase::database(m_connection, false).isOpen();
}

QVariant Database::value(const QString &key, const QVariant &defaultValue) const
{
    if (!isOpen())
        return defaultValue;
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    q.prepare(QStringLiteral("SELECT value FROM settings WHERE key = ?"));
    q.addBindValue(key);
    if (q.exec() && q.next())
        return q.value(0);
    return defaultValue;
}

void Database::setValue(const QString &key, const QVariant &value)
{
    if (!isOpen())
        return;
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    q.prepare(QStringLiteral("INSERT INTO settings(key, value) VALUES(?, ?) "
                             "ON CONFLICT(key) DO UPDATE SET value = excluded.value"));
    q.addBindValue(key);
    q.addBindValue(value.toString());
    if (q.exec())
        emit valueChanged(key);
}

QString Database::normalised(const QString &path)
{
    return QDir::cleanPath(QDir::fromNativeSeparators(path));
}

QList<Database::RecentProject> Database::recentProjects() const
{
    QList<RecentProject> out;
    if (!isOpen())
        return out;
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    q.exec(QStringLiteral("SELECT name, path, last_opened FROM recent_projects ORDER BY last_opened DESC"));
    while (q.next()) {
        out.push_back({q.value(0).toString(), q.value(1).toString(),
                       QDateTime::fromMSecsSinceEpoch(q.value(2).toLongLong())});
    }
    return out;
}

void Database::addRecentProject(const QString &path)
{
    if (!isOpen())
        return;
    const QString p = normalised(path);
    const QString name = QFileInfo(p).fileName().isEmpty() ? p : QFileInfo(p).fileName();
    QSqlDatabase db = QSqlDatabase::database(m_connection, false);
    QSqlQuery q(db);
    q.prepare(QStringLiteral("INSERT INTO recent_projects(path, name, last_opened) VALUES(?, ?, ?) "
                             "ON CONFLICT(path) DO UPDATE SET last_opened = excluded.last_opened, name = excluded.name"));
    q.addBindValue(p);
    q.addBindValue(name);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    q.exec();
    QSqlQuery trim(db);
    trim.prepare(QStringLiteral("DELETE FROM recent_projects WHERE path NOT IN "
                                "(SELECT path FROM recent_projects ORDER BY last_opened DESC LIMIT ?)"));
    trim.addBindValue(kMaxRecent);
    trim.exec();
    emit recentProjectsChanged();
}

void Database::removeRecentProject(const QString &path)
{
    if (!isOpen())
        return;
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    q.prepare(QStringLiteral("DELETE FROM recent_projects WHERE path = ?"));
    q.addBindValue(normalised(path));
    q.exec();
    emit recentProjectsChanged();
}

void Database::clearRecentProjects()
{
    if (!isOpen())
        return;
    QSqlQuery q(QSqlDatabase::database(m_connection, false));
    q.exec(QStringLiteral("DELETE FROM recent_projects")); // never touches the project folders themselves
    emit recentProjectsChanged();
}

} // namespace paw
