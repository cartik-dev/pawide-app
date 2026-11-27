#pragma once
#include <QList>
#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;

namespace paw {

// Offline plugin catalog shown in the Extensions side view. Installing is
// local-only for now (the plugin runtime lands in a later phase), but the
// catalogue, search and installed state are real and persist in the database.
class ExtensionsPanel : public QWidget {
    Q_OBJECT
public:
    struct PluginInfo {
        QString id, name, author, version, description;
    };
    explicit ExtensionsPanel(QWidget *parent = nullptr);

    QStringList installedIds() const;
    void refresh();

signals:
    void logMessage(const QString &line);

private:
    void rebuildList(const QString &filter = {});
    void toggleInstall(const QString &id);

    QList<PluginInfo> m_catalog;
    QListWidget *m_list = nullptr;
    QLineEdit *m_search = nullptr;
    QLabel *m_summary = nullptr;
    QStringList m_installed;
};

} // namespace paw
