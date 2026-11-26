#pragma once
#include <QObject>
#include <QPointer>

class QNetworkAccessManager;
class QNetworkReply;

namespace paw {

// Checks GitHub Releases for a newer version (SemVer comparison of tag_name).
// The updater never downloads or installs anything by itself; it only opens
// the releases page.
class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);

    void check(bool silent); // silent = no message when already up to date

signals:
    void updateAvailable(const QString &latestVersion, const QString &url);
    void upToDate();
    void failed(const QString &error);

private:
    static int versionCode(const QString &version); // "1.2.3" -> 10203

    QNetworkAccessManager *m_nam = nullptr;
};
