#include "UpdateChecker.h"

#include "core/AppInfo.h"

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace paw {

namespace {
const QUrl releasesApiUrl = QStringLiteral("https://api.github.com/repos/cartik-dev/pawide-app/releases/latest");
}

UpdateChecker::UpdateChecker(QObject *parent) : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
}

void UpdateChecker::check(bool silent)
{
    QNetworkRequest req(releasesApiUrl);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("PawIDE/%1").arg(appVersion()));
    req.setTransferTimeout(15000);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, silent] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            if (!silent)
                emit failed(reply->errorString());
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QString tag = doc.object().value(QStringLiteral("tag_name")).toString();
        const QString htmlUrl = doc.object().value(QStringLiteral("html_url")).toString();
        if (tag.isEmpty()) {
            if (!silent)
                emit failed(tr("The releases feed returned no version tag."));
            return;
        }
        if (versionCode(tag) > versionCode(appVersion())) {
            const QString url = htmlUrl.isEmpty() ? repositoryUrl() + QStringLiteral("/releases/latest") : htmlUrl;
            emit updateAvailable(tag, url);
        } else {
            emit upToDate();
        }
    });
}

int UpdateChecker::versionCode(const QString &version)
{
    QString v = version;
    v.remove(QLatin1Char('v'));
    int code = 0;
    for (const QString &s : v.split(QLatin1Char('.'))) {
        code = code * 100 + qBound(0, s.toInt(), 99);
        if (code > 99999999)
            break;
    }
    return code;
}

} // namespace paw
