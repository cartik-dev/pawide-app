#include "core/FileUtils.h"
#include "core/Language.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace paw;

class TstFileUtils : public QObject {
    Q_OBJECT
private slots:
    void utf8Plain()
    {
        const TextFile f = FileUtils::decode(QByteArray("hello\nworld\n"));
        QVERIFY(f.ok);
        QCOMPARE(f.encoding, QString("UTF-8"));
        QCOMPARE(f.eol, LineEnding::LF);
        QVERIFY(!f.mixedEol);
        QCOMPARE(f.text, QString("hello\nworld\n"));
    }
    void utf8Cyrillic()
    {
        const QString s = QStringLiteral("Привет, мир! 日本語 🐾");
        const TextFile f = FileUtils::decode(s.toUtf8());
        QVERIFY(f.ok);
        QCOMPARE(f.encoding, QString("UTF-8"));
        QCOMPARE(f.text, s);
    }
    void utf8Bom()
    {
        const TextFile f = FileUtils::decode(QByteArray("\xEF\xBB\xBF") + QByteArray("abc"));
        QCOMPARE(f.encoding, QString("UTF-8 BOM"));
        QCOMPARE(f.text, QString("abc"));
    }
    void utf16RoundTrip_data()
    {
        QTest::addColumn<QString>("enc");
        QTest::newRow("le") << "UTF-16 LE";
        QTest::newRow("be") << "UTF-16 BE";
        QTest::newRow("bom") << "UTF-8 BOM";
        QTest::newRow("utf8") << "UTF-8";
    }
    void utf16RoundTrip()
    {
        QFETCH(QString, enc);
        const QString s = QStringLiteral("line1 Привет\nline2 ✓\n");
        const QByteArray bytes = FileUtils::encode(s, enc, LineEnding::CRLF);
        const TextFile f = FileUtils::decode(bytes);
        QVERIFY(f.ok);
        QCOMPARE(f.encoding, enc);
        QCOMPARE(f.eol, LineEnding::CRLF);
        QCOMPARE(f.text, s);
    }
    void latin1Fallback()
    {
        const QByteArray bytes("caf\xE9\n"); // invalid UTF-8
        const TextFile f = FileUtils::decode(bytes);
        QVERIFY(f.ok);
        QCOMPARE(f.encoding, QString("Latin-1"));
        QCOMPARE(f.text, QString::fromUtf8("caf\xC3\xA9\n"));
        bool lossless = false;
        QCOMPARE(FileUtils::encode(f.text, f.encoding, LineEnding::LF, &lossless), bytes);
        QVERIFY(lossless);
    }
    void lineEndingDetection()
    {
        QCOMPARE(FileUtils::decode("a\r\nb\r\nc\r\n").eol, LineEnding::CRLF);
        QCOMPARE(FileUtils::decode("a\nb\nc\n").eol, LineEnding::LF);
        QCOMPARE(FileUtils::decode("a\rb\rc\r").eol, LineEnding::CR);
        const TextFile mixed = FileUtils::decode("a\r\nb\r\nc\nd\n");
        QVERIFY(mixed.mixedEol);
        QCOMPARE(mixed.eol, LineEnding::CRLF);
        QVERIFY(!mixed.text.contains(QLatin1Char('\r')));
    }
    void binaryDetection()
    {
        QByteArray bin("MZ\x90\x00\x03\x00\x00\x00", 8);
        QVERIFY(FileUtils::looksBinary(bin));
        const TextFile f = FileUtils::decode(bin);
        QVERIFY(!f.ok);
        QVERIFY(f.binary);
        QVERIFY(!FileUtils::looksBinary("plain text\twith tab\n"));
        QVERIFY(FileUtils::decode(QByteArray()).ok); // empty file is fine
    }
    void writeKeepsBytes_data()
    {
        QTest::addColumn<QByteArray>("original");
        QTest::newRow("lf") << QByteArray("one\ntwo\n");
        QTest::newRow("crlf") << QByteArray("one\r\ntwo\r\n");
        QTest::newRow("bom-crlf") << (QByteArray("\xEF\xBB\xBF") + "one\r\ntwo\r\n");
        QTest::newRow("no-final-newline") << QByteArray("one\ntwo");
        QTest::newRow("cyrillic") << QString::fromUtf8("Привет\nмир\n").toUtf8();
    }
    void writeKeepsBytes()
    {
        QFETCH(QByteArray, original);
        QTemporaryDir dir;
        const QString path = dir.filePath("f.txt");
        QFile out(path);
        QVERIFY(out.open(QIODevice::WriteOnly));
        out.write(original);
        out.close();
        const TextFile f = FileUtils::readText(path);
        QVERIFY(f.ok);
        QString enc = f.encoding;
        QString err;
        QVERIFY2(FileUtils::writeText(path, f.text, enc, f.eol, &err), qPrintable(err));
        QFile in(path);
        QVERIFY(in.open(QIODevice::ReadOnly));
        QCOMPARE(in.readAll(), original); // a read/save cycle must not change the file
    }
    void unrepresentableFallsBackToUtf8()
    {
        QTemporaryDir dir;
        const QString path = dir.filePath("f.txt");
        QString enc = "Latin-1";
        QString err;
        QVERIFY(FileUtils::writeText(path, QString::fromUtf8("caf\xC3\xA9 \xE2\x9C\x93"), enc, LineEnding::LF, &err));
        QCOMPARE(enc, QString("UTF-8")); // never silently replaced by '?'
        QCOMPARE(FileUtils::readText(path).text, QString::fromUtf8("caf\xC3\xA9 \xE2\x9C\x93"));
    }
    void readMissingFile()
    {
        const TextFile f = FileUtils::readText("/definitely/not/here.txt");
        QVERIFY(!f.ok);
        QVERIFY(!f.error.isEmpty());
    }
    void languageDetection_data()
    {
        QTest::addColumn<QString>("file");
        QTest::addColumn<QString>("id");
        QTest::newRow("lua") << "main.lua" << "lua";
        QTest::newRow("rust") << "src/main.rs" << "rust";
        QTest::newRow("python") << "app.py" << "python";
        QTest::newRow("cpp") << "x.cpp" << "cpp";
        QTest::newRow("hpp") << "x.hpp" << "cpp";
        QTest::newRow("json") << "settings.json" << "json";
        QTest::newRow("toml") << "Cargo.toml" << "toml";
        QTest::newRow("cmake") << "CMakeLists.txt" << "cmake";
        QTest::newRow("dockerfile") << "Dockerfile" << "dockerfile";
        QTest::newRow("makefile") << "Makefile" << "makefile";
        QTest::newRow("dotenv") << ".env" << "dotenv";
        QTest::newRow("dotenv-local") << ".env.local" << "dotenv";
        QTest::newRow("gitignore") << ".gitignore" << "ignore";
        QTest::newRow("powershell") << "run.ps1" << "powershell";
        QTest::newRow("unknown") << "data.xyz" << "plaintext";
        QTest::newRow("noext") << "LICENSE" << "plaintext";
    }
    void languageDetection()
    {
        QFETCH(QString, file);
        QFETCH(QString, id);
        QCOMPARE(Language::forFile(file).id, id);
    }
};

QTEST_GUILESS_MAIN(TstFileUtils)
#include "tst_fileutils.moc"
