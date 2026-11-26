#pragma once
#include <QWidget>

class QListWidget;
class QPushButton;
class QLabel;

namespace paw {

class WelcomeWidget : public QWidget {
    Q_OBJECT
public:
    explicit WelcomeWidget(QWidget *parent = nullptr);
    void reload(); // refresh the recent projects list

signals:
    void openFolderRequested();
    void newProjectRequested();
    void openRecentRequested(const QString &path);
    void documentationRequested();

private:
    QListWidget *m_list;
    QLabel *m_recentTitle;
    QLabel *m_empty;
    QPushButton *m_clear;
};

} // namespace paw
