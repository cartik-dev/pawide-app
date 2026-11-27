#include "core/AppContext.h"
#include "core/AppInfo.h"
#include "core/PluginManager.h"
#include "ui/Icons.h"
#include "ui/MainWindow.h"
#include "ui/Theme.h"

#include <QApplication>
#include <QFileInfo>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("Paw"));
    QApplication::setApplicationName(QStringLiteral("PawIDE"));
    QApplication::setApplicationDisplayName(paw::appName());
    QApplication::setApplicationVersion(paw::appVersion());
    QApplication::setWindowIcon(paw::icons::appIcon());

    // Plugins: translators must be installed before any window is created,
    // otherwise the tr() calls have already produced untranslated strings.
    paw::PluginManager::instance().scan();
    paw::PluginManager::instance().activate();

    // PAWIDE_DB lets tests and portable setups relocate the settings database.
    const QByteArray dbOverride = qgetenv("PAWIDE_DB");
    if (!dbOverride.isEmpty())
        paw::setDatabasePath(QString::fromLocal8Bit(dbOverride));

    paw::theme::apply(app);

    auto *window = paw::MainWindow::createWindow();
    window->show();

    // Command line: PawIDE [folder | file ...]
    const QStringList args = app.arguments().mid(1);
    bool projectOpened = false;
    for (const QString &arg : args) {
        const QFileInfo fi(arg);
        if (fi.isDir() && !projectOpened) {
            projectOpened = window->openProject(fi.absoluteFilePath());
        } else if (fi.isFile()) {
            window->openFile(fi.absoluteFilePath());
        }
    }
    return app.exec();
}
