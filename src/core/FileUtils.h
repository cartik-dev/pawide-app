#pragma once
#include <QByteArray>
#include <QString>

namespace paw {

enum class LineEnding { LF, CRLF, CR };

struct TextFile {
    bool ok = false;
    bool binary = false;
    QString error;
    QString text;                    // always normalised to "\n"
    QString encoding = "UTF-8";      // "UTF-8", "UTF-8 BOM", "UTF-16 LE", "UTF-16 BE", "Latin-1"
    LineEnding eol = LineEnding::LF; // dominant line ending of the file
    bool mixedEol = false;           // file contained more than one kind
    qint64 size = 0;
};

namespace FileUtils {
QString eolName(LineEnding e);
bool looksBinary(const QByteArray &data);
TextFile decode(const QByteArray &data);
TextFile readText(const QString &path);
// Encodes text ("\n" separated) with the given encoding and line ending.
// Sets *lossless=false if the encoding cannot represent the text (caller should fall back).
QByteArray encode(const QString &text, const QString &encoding, LineEnding eol, bool *lossless = nullptr);
// Atomic write (QSaveFile). On success the file is either fully written or untouched.
bool writeText(const QString &path, const QString &text, QString &encoding, LineEnding eol, QString *error);
} // namespace FileUtils

} // namespace paw
