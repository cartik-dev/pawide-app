#include "FileUtils.h"

#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStringConverter>

namespace paw {
namespace FileUtils {

QString eolName(LineEnding e)
{
    switch (e) {
    case LineEnding::CRLF: return QStringLiteral("CRLF");
    case LineEnding::CR: return QStringLiteral("CR");
    default: return QStringLiteral("LF");
    }
}

bool looksBinary(const QByteArray &data)
{
    const int n = static_cast<int>(qMin<qsizetype>(data.size(), 8000));
    if (n == 0)
        return false;
    int control = 0;
    for (int i = 0; i < n; ++i) {
        const unsigned char c = static_cast<unsigned char>(data[i]);
        if (c == 0)
            return true;
        if ((c < 9) || (c > 13 && c < 32 && c != 27))
            ++control;
    }
    return control * 10 > n; // more than 10% control characters
}

static void normaliseEol(QString &text, LineEnding &eol, bool &mixed)
{
    qsizetype crlf = 0, lf = 0, cr = 0;
    const qsizetype len = text.size();
    for (qsizetype i = 0; i < len; ++i) {
        const QChar c = text.at(i);
        if (c == QLatin1Char('\r')) {
            if (i + 1 < len && text.at(i + 1) == QLatin1Char('\n')) {
                ++crlf;
                ++i;
            } else {
                ++cr;
            }
        } else if (c == QLatin1Char('\n')) {
            ++lf;
        }
    }
    eol = LineEnding::LF;
    if (crlf >= lf && crlf >= cr && crlf > 0)
        eol = LineEnding::CRLF;
    else if (cr > lf && cr > crlf)
        eol = LineEnding::CR;
    mixed = ((crlf > 0) + (lf > 0) + (cr > 0)) > 1;
    if (crlf > 0 || cr > 0) {
        text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
        text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    }
}

TextFile decode(const QByteArray &data)
{
    TextFile r;
    r.size = data.size();

    if (data.startsWith("\xEF\xBB\xBF")) {
        r.encoding = QStringLiteral("UTF-8 BOM");
        r.text = QString::fromUtf8(data.constData() + 3, data.size() - 3);
    } else if (data.startsWith("\xFF\xFE")) {
        r.encoding = QStringLiteral("UTF-16 LE");
        QStringDecoder dec(QStringConverter::Utf16LE);
        r.text = dec(QByteArrayView(data).sliced(2));
    } else if (data.startsWith("\xFE\xFF")) {
        r.encoding = QStringLiteral("UTF-16 BE");
        QStringDecoder dec(QStringConverter::Utf16BE);
        r.text = dec(QByteArrayView(data).sliced(2));
    } else {
        if (looksBinary(data)) {
            r.binary = true;
            r.error = QStringLiteral("The file appears to be binary and cannot be opened as text.");
            return r;
        }
        QStringDecoder dec(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
        const QString s = dec(data);
        if (!dec.hasError()) {
            r.encoding = QStringLiteral("UTF-8");
            r.text = s;
        } else {
            r.encoding = QStringLiteral("Latin-1");
            r.text = QString::fromLatin1(data);
        }
    }
    normaliseEol(r.text, r.eol, r.mixedEol);
    r.ok = true;
    return r;
}

TextFile readText(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        TextFile r;
        r.error = QStringLiteral("Cannot open file: %1").arg(f.errorString());
        return r;
    }
    return decode(f.readAll());
}

QByteArray encode(const QString &textIn, const QString &encoding, LineEnding eol, bool *lossless)
{
    if (lossless)
        *lossless = true;
    QString text = textIn;
    if (eol == LineEnding::CRLF)
        text.replace(QLatin1Char('\n'), QStringLiteral("\r\n"));
    else if (eol == LineEnding::CR)
        text.replace(QLatin1Char('\n'), QLatin1Char('\r'));

    if (encoding == QLatin1String("UTF-8 BOM"))
        return QByteArray("\xEF\xBB\xBF") + text.toUtf8();
    if (encoding == QLatin1String("UTF-16 LE")) {
        QStringEncoder enc(QStringConverter::Utf16LE);
        return QByteArray("\xFF\xFE") + QByteArray(enc(text));
    }
    if (encoding == QLatin1String("UTF-16 BE")) {
        QStringEncoder enc(QStringConverter::Utf16BE);
        return QByteArray("\xFE\xFF") + QByteArray(enc(text));
    }
    if (encoding == QLatin1String("Latin-1")) {
        for (const QChar c : text) {
            if (c.unicode() > 0xFF) {
                if (lossless)
                    *lossless = false;
                break;
            }
        }
        return text.toLatin1();
    }
    return text.toUtf8();
}

bool writeText(const QString &path, const QString &text, QString &encoding, LineEnding eol, QString *error)
{
    bool lossless = true;
    QByteArray bytes = encode(text, encoding, eol, &lossless);
    if (!lossless) {
        // Never silently corrupt characters: fall back to UTF-8.
        encoding = QStringLiteral("UTF-8");
        bytes = encode(text, encoding, eol);
    }
    const QFileDevice::Permissions oldPerms =
        QFileInfo::exists(path) ? QFile::permissions(path) : QFileDevice::Permissions();
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QStringLiteral("Cannot write file: %1").arg(f.errorString());
        return false;
    }
    if (f.write(bytes) != bytes.size()) {
        if (error)
            *error = QStringLiteral("Write failed: %1").arg(f.errorString());
        f.cancelWriting();
        return false;
    }
    if (oldPerms)
        f.setPermissions(oldPerms);
    if (!f.commit()) {
        if (error)
            *error = QStringLiteral("Cannot finalise file: %1").arg(f.errorString());
        return false;
    }
    return true;
}

} // namespace FileUtils
} // namespace paw
