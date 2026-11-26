#pragma once
#include <QDialog>
#include <functional>

class QLineEdit;
class QListWidget;

namespace paw {

struct PaletteItem {
    QString title;
    QString detail; // shortcut, path, ...
    std::function<void()> run;
    bool enabled = true;
};

// Popup used for the Command Palette (Ctrl+Shift+P) and Quick Open (Ctrl+P).
class CommandPalette : public QDialog {
    Q_OBJECT
public:
    explicit CommandPalette(QWidget *parent = nullptr);
    void showItems(const QList<PaletteItem> &items, const QString &placeholder);

    // Exposed for tests.
    static bool fuzzyMatch(const QString &query, const QString &text, int *score = nullptr);
    int visibleCount() const;
    void setQuery(const QString &q);
    void runCurrent();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void refilter();

    QLineEdit *m_edit;
    QListWidget *m_list;
    QList<PaletteItem> m_items;
};

} // namespace paw
