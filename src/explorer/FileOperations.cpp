#include "FileOperations.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

namespace paw::fileops {

bool validateName(const QString &name, QString *error)
{
    auto fail = [&](const QString &msg) {
        if (error)
            *error = msg;
        return false;
    };
    if (name.trimmed().isEmpty())
        return fail(QStringLiteral("The name cannot be empty."));
    if (name == QLatin1String(".") || name == QLatin1String(".."))
        return fail(QStringLiteral("This name is not allowed."));
    if (name.size() > 255)
        return fail(QStringLiteral("The name is too long."));
    static const QString bad = QStringLiteral("<>:\"/\\|?*");
    for (const QChar c : name) {
        if (bad.contains(c) || c.unicode() < 32)
            return fail(QStringLiteral("The name contains an invalid character: %1").arg(c));
    }
    if (name.endsWith(QLatin1Char(' ')) || name.endsWith(QLatin1Char('.')))
        return fail(QStringLiteral("The name cannot end with a space or a dot."));
    static const QSet<QString> reserved = {
        "con", "prn", "aux", "nul", "com1", "com2", "com3", "com4", "com5", "com6", "com7", "com8", "com9",
        "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9"};
    const QString stem = name.left(name.indexOf(QLatin1Char('.')) < 0 ? name.size() : name.indexOf(QLatin1Char('.'))).toLower();
    if (reserved.contains(stem))
        return fail(QStringLiteral("'%1' is a reserved name on Windows.").arg(name));
    return true;
}

QString uniqueCopyName(const QString &dir, const QString &fileName)
{
    const QDir d(dir);
    const QFileInfo fi(fileName);
    // Dot-files such as ".env" have no "base name" for our purposes.
    QString base = fi.completeBaseName();
    QString ext = fi.suffix();
    if (base.isEmpty()) {
        base = fileName;
        ext.clear();
    }
    const QString dotExt = ext.isEmpty() ? QString() : QStringLiteral(".") + ext;
    QString candidate = base + QStringLiteral(" copy") + dotExt;
    int n = 2;
    while (QFileInfo::exists(d.filePath(candidate)))
        candidate = base + QStringLiteral(" copy %1").arg(n++) + dotExt;
    return candidate;
}

bool createFile(const QString &path, QString *error)
{
    if (QFileInfo::exists(path)) {
        if (error)
            *error = QStringLiteral("A file or folder with this name already exists.");
        return false;
    }
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error)
            *error = f.errorString();
        return false;
    }
    return true;
}

bool createFolder(const QString &path, QString *error)
{
    if (QFileInfo::exists(path)) {
        if (error)
            *error = QStringLiteral("A file or folder with this name already exists.");
        return false;
    }
    if (!QDir().mkpath(path)) {
        if (error)
            *error = QStringLiteral("Cannot create the folder.");
        return false;
    }
    return true;
}

bool copyRecursively(const QString &src, const QString &dst, QString *error)
{
    const QFileInfo si(src);
    if (!si.exists()) {
        if (error)
            *error = QStringLiteral("The source no longer exists.");
        return false;
    }
    if (QFileInfo::exists(dst)) {
        if (error)
            *error = QStringLiteral("The destination already exists.");
        return false;
    }
    if (!si.isDir()) {
        if (!QFile::copy(src, dst)) {
            if (error)
                *error = QStringLiteral("Cannot copy '%1'.").arg(si.fileName());
            return false;
        }
        return true;
    }
    // Refuse to copy a folder into itself (would recurse forever).
    const QString srcCanon = si.canonicalFilePath() + QLatin1Char('/');
    const QString dstParent = QFileInfo(dst).dir().canonicalPath() + QLatin1Char('/');
    if (dstParent.startsWith(srcCanon)) {
        if (error)
            *error = QStringLiteral("Cannot copy a folder into itself.");
        return false;
    }
    if (!QDir().mkpath(dst)) {
        if (error)
            *error = QStringLiteral("Cannot create the folder '%1'.").arg(dst);
        return false;
    }
    const QDir sd(src);
    const auto entries = sd.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
    for (const QFileInfo &e : entries) {
        if (!copyRecursively(e.filePath(), QDir(dst).filePath(e.fileName()), error))
            return false;
    }
    return true;
}

bool removePath(const QString &path, QString *error)
{
    const QFileInfo fi(path);
    bool ok;
    if (fi.isDir() && !fi.isSymLink())
        ok = QDir(path).removeRecursively();
    else
        ok = QFile::remove(path);
    if (!ok && error)
        *error = QStringLiteral("Cannot delete '%1'. It may be in use or protected.").arg(fi.fileName());
    return ok;
}

bool renamePath(const QString &path, const QString &newName, QString *error)
{
    if (!validateName(newName, error))
        return false;
    const QFileInfo fi(path);
    const QString target = fi.dir().filePath(newName);
    if (target != path && QFileInfo::exists(target)) {
        // Case-only renames on case-insensitive filesystems are fine; everything else is a clash.
        if (QFileInfo(target).canonicalFilePath() != fi.canonicalFilePath()) {
            if (error)
                *error = QStringLiteral("'%1' already exists.").arg(newName);
            return false;
        }
    }
    if (!QDir().rename(path, target)) {
        if (error)
            *error = QStringLiteral("Cannot rename '%1'.").arg(fi.fileName());
        return false;
    }
    return true;
}

} // namespace paw::fileops
