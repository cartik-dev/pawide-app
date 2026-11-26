#pragma once
#include <QIcon>
#include <QMap>
#include <QWidget>

class QToolButton;
class QVBoxLayout;

namespace paw {

// Left vertical bar with view switchers (Explorer, Search, ...). Plugins will add items here.
class ActivityBar : public QWidget {
    Q_OBJECT
public:
    explicit ActivityBar(QWidget *parent = nullptr);

    void addItem(const QString &id, const QString &title, const QIcon &icon);
    void addBottomAction(const QString &id, const QString &title, const QIcon &icon);
    void setCurrent(const QString &id);
    QString current() const { return m_current; }

signals:
    void viewSelected(const QString &id);   // a different view was chosen
    void activeViewClicked(const QString &id); // the already active view was clicked again
    void bottomActionTriggered(const QString &id);

private:
    QToolButton *makeButton(const QString &id, const QString &title, const QIcon &icon, bool checkable);

    QVBoxLayout *m_top;
    QVBoxLayout *m_bottom;
    QMap<QString, QToolButton *> m_buttons;
    QString m_current;
};

} // namespace paw
