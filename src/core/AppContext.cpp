#include "AppContext.h"

#include "database/Database.h"

#include <memory>

namespace paw {

static QString &configuredPath()
{
    static QString p;
    return p;
}

void setDatabasePath(const QString &path) { configuredPath() = path; }

Database &appDatabase()
{
    static std::unique_ptr<Database> db = [] {
        auto d = std::make_unique<Database>();
        const QString path = configuredPath().isEmpty() ? Database::defaultPath() : configuredPath();
        d->open(path);
        return d;
    }();
    return *db;
}

} // namespace paw
