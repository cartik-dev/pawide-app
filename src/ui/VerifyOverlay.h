#pragma once
#include <QWidget>

class QLabel;

namespace paw {

// Non-intrusive card (bottom-right of the editor) that reports the sandbox
// verification of a "not sure" suspicion: started → confirmed / false alarm.
class VerifyOverlay : public QWidget {
    Q_OBJECT
public:
    explicit VerifyOverlay(QWidget *parent = nullptr);

    void showRunning(const QString &issue, int line);
    void showConfirmed(const QString &issue, int line, const QString &detail);
    void showFalseAlarm(const QString &issue, int line);
    void showSureError(const QString &issue, int line);
    void hideSoon(int ms = 9000);
    void reposition();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void paintCard(const QColor &accent, const QString &title, const QString &body, const QString &badge);

    QLabel *m_icon = nullptr;
    QLabel *m_title = nullptr;
    QLabel *m_body = nullptr;
    QLabel *m_badge = nullptr;
    bool m_filterInstalled = false;
};

} // namespace paw
