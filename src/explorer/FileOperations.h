#pragma once
#include <QString>

namespace paw::fileops {

// Validates a single file or folder name (not a path). Windows rules are applied on all platforms.
bool validateName(const QString &name, QString *error);
// "name copy.ext", "name copy 2.ext", ... that does not exist yet inside `dir`.
QString uniqueCopyName(const QString &dir, const QString &fileName);
bool createFile(const QString &path, QString *error);
bool createFolder(const QString &path, QString *error);
bool copyRecursively(const QString &src, const QString &dst, QString *error);
bool removePath(const QString &path, QString *error);
bool renamePath(const QString &path, const QString &newName, QString *error);

} // namespace paw::fileops
