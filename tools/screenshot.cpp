// Developer tool: renders the main window to a PNG (works with QT_QPA_PLATFORM=offscreen).
//   pawide_screenshot <out.png> welcome
//   pawide_screenshot <out.png> project <folder> [file-to-open] [palette]
#include "core/AppContext.h"
#include "ui/CommandPalette.h"
#include "ui/Icons.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QEventLoop>
#include <QTimer>

static void wait(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    if (argc < 3)
        return 2;
    paw::setDatabasePath(QStringLiteral(":memory:"));
    paw::theme::apply(app);
    paw::MainWindow w;
    w.resize(1320, 820);
    w.show();
    const QString mode = QString::fromLocal8Bit(argv[2]);
    if (mode == QLatin1String("project") && argc >= 4) {
        w.openProject(QString::fromLocal8Bit(argv[3]));
        wait(400);
        if (argc >= 5)
            w.openFile(QString::fromLocal8Bit(argv[4]));
        if (argc >= 6)
            w.palette()->showItems({{"File: Open Folder…", "Ctrl+Shift+O", {}, true}, {"File: New Project…", "", {}, true},
                                    {"View: Toggle Panel", "Ctrl+J", {}, true}}, "Type a command");
    }
    wait(900);
    return w.grab().save(QString::fromLocal8Bit(argv[1])) ? 0 : 1;
}
