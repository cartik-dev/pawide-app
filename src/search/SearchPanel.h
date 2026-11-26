#pragma once
#include <QWidget>

class QCheckBox;
class QLabel;
class QLineEdit;
class QTreeWidget;
class QTreeWidgetItem;

namespace paw {

// Side-bar page with project-wide text search. The scan runs off the GUI
// thread; results are grouped by file, a click opens the file at the line.
class SearchPanel : public QWidget {
    Q_OBJECT
public:
    explicit SearchPanel(QWidget *parent = nullptr);

    void setRootPath(const QString &path);

signals:
    void resultActivated(const QString &path, int line);
    void logMessage(const QString &line);

protected:
    void showEvent(QShowEvent *e) override;

private:
    void runSearch();

    QString m_root;
    QLineEdit *m_query = nullptr;
    QCheckBox *m_case = nullptr;
    QCheckBox *m_whole = nullptr;
    QLabel *m_summary = nullptr;
    QTreeWidget *m_results = nullptr;
    bool m_busy = false;
};

} // namespace paw
