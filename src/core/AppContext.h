#pragma once
#include <QString>

namespace paw {
class Database;

// Process-wide database. Call setDatabasePath() before the first appDatabase()
// (tests use ":memory:").
void setDatabasePath(const QString &path);
Database &appDatabase();
} // namespace paw
