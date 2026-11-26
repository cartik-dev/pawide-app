#include "explorer/FileOperations.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using namespace paw;

static void write(const QString &path, const QByteArray &data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

class TstFileOps : public QObject {
    Q_OBJECT
private slots:
    void names_data()
    {
        QTest::addColumn<QString>("name");
        QTest::addColumn<bool>("valid");
        QTest::newRow("plain") << "main.cpp" << true;
        QTest::newRow("dotfile") << ".env.local" << true;
        QTest::newRow("noext") << "Dockerfile" << true;
        QTest::newRow("unicode") << QString::fromUtf8("файл.txt") << true;
        QTest::newRow("spaces inside") << "my file.txt" << true;
        QTest::newRow("empty") << "" << false;
        QTest::newRow("spaces only") << "   " << false;
        QTest::newRow("slash") << "a/b" << false;
        QTest::newRow("question") << "a?b" << false;
        QTest::newRow("pipe") << "a|b" << false;
        QTest::newRow("trailing space") << "abc " << false;
        QTest::newRow("com1") << "COM1" << false;
        QTest::newRow("lpt1.txt") << "lpt1.txt" << false;
        QTest::newRow("dot") << "." << false;
        QTest::newRow("too long") << QString(300, 'a') << false;
    }
    void names()
    {
        QFETCH(QString, name);
        QFETCH(bool, valid);
        QString err;
        QCOMPARE(fileops::validateName(name, &err), valid);
        QCOMPARE(err.isEmpty(), valid);
    }
    void uniqueCopyNames()
    {
        QTemporaryDir tmp;
        write(tmp.filePath("a.txt"), "x");
        QCOMPARE(fileops::uniqueCopyName(tmp.path(), "a.txt"), QString("a copy.txt"));
        write(tmp.filePath("a copy.txt"), "x");
        QCOMPARE(fileops::uniqueCopyName(tmp.path(), "a.txt"), QString("a copy 2.txt"));
        QCOMPARE(fileops::uniqueCopyName(tmp.path(), ".env"), QString(".env copy"));
        QCOMPARE(fileops::uniqueCopyName(tmp.path(), "Makefile"), QString("Makefile copy"));
    }
    void createAndRemove()
    {
        QTemporaryDir tmp;
        QString err;
        const QString f = tmp.filePath(".hidden");
        QVERIFY(fileops::createFile(f, &err));
        QVERIFY(!fileops::createFile(f, &err)); // never overwrites
        QVERIFY(!err.isEmpty());
        const QString d = tmp.filePath("a/b/c");
        QVERIFY(fileops::createFolder(d, &err));
        write(d + "/file.txt", "x");
        QVERIFY(fileops::removePath(tmp.filePath("a"), &err));
        QVERIFY(!QFileInfo::exists(tmp.filePath("a")));
        QVERIFY(fileops::removePath(f, &err));
    }
    void copyRecursivelyIncludesHidden()
    {
        QTemporaryDir tmp;
        write(tmp.filePath("src/main.cpp"), "int main(){}");
        write(tmp.filePath("src/.config/settings.json"), "{}");
        write(tmp.filePath("src/.env"), "KEY=1");
        QString err;
        QVERIFY2(fileops::copyRecursively(tmp.filePath("src"), tmp.filePath("dst"), &err), qPrintable(err));
        QVERIFY(QFileInfo(tmp.filePath("dst/main.cpp")).isFile());
        QVERIFY(QFileInfo(tmp.filePath("dst/.config/settings.json")).isFile());
        QVERIFY(QFileInfo(tmp.filePath("dst/.env")).isFile());
        QVERIFY(!fileops::copyRecursively(tmp.filePath("src"), tmp.filePath("dst"), &err)); // exists
    }
    void copyIntoItselfRefused()
    {
        QTemporaryDir tmp;
        write(tmp.filePath("src/a.txt"), "x");
        QString err;
        QVERIFY(!fileops::copyRecursively(tmp.filePath("src"), tmp.filePath("src/inner"), &err));
        QVERIFY(!QFileInfo::exists(tmp.filePath("src/inner")));
    }
    void renameRules()
    {
        QTemporaryDir tmp;
        write(tmp.filePath("a.txt"), "a");
        write(tmp.filePath("b.txt"), "b");
        QString err;
        QVERIFY(!fileops::renamePath(tmp.filePath("a.txt"), "b.txt", &err)); // clash
        QVERIFY(!fileops::renamePath(tmp.filePath("a.txt"), "bad/name", &err));
        QVERIFY(fileops::renamePath(tmp.filePath("a.txt"), "c.txt", &err));
        QVERIFY(QFileInfo(tmp.filePath("c.txt")).isFile());
        QVERIFY(fileops::createFolder(tmp.filePath("dir"), &err));
        QVERIFY(fileops::renamePath(tmp.filePath("dir"), "dir2", &err)); // folders too
        QVERIFY(QFileInfo(tmp.filePath("dir2")).isDir());
    }
};

QTEST_APPLESS_MAIN(TstFileOps)
#include "tst_fileops.moc"
