#include "VerifyOverlay.h"

#include "ui/Icons.h"

#include <QEvent>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>

namespace paw {

VerifyOverlay::VerifyOverlay(QWidget *parent) : QWidget(parent)
{
    setWindowFlags(Qt::Widget);
    setAttribute(Qt::WA_StyledBackground, true);
    setStyleSheet(QStringLiteral(
        "VerifyOverlay { background: #12121a; border: 1px solid #3a2a6b; border-radius: 12px; }"
        "QLabel { background: transparent; }"));
    auto *effect = new QGraphicsDropShadowEffect(this);
    effect->setBlurRadius(28);
    effect->setOffset(0, 6);
    effect->setColor(QColor(0, 0, 0, 160));
    setGraphicsEffect(effect);

    setFixedWidth(440);
    setFixedHeight(96);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 10, 14, 10);
    layout->setSpacing(12);

    m_icon = new QLabel(this);
    m_icon->setFixedSize(34, 34);
    m_icon->setScaledContents(true);
    layout->addWidget(m_icon, 0, Qt::AlignTop);

    auto *textLayout = new QVBoxLayout;
    textLayout->setSpacing(3);
    m_title = new QLabel(this);
    m_title->setStyleSheet(QStringLiteral("QLabel { color: #e4e4e7; font-weight: 600; font-size: 13px; }"));
    m_title->setWordWrap(true);
    m_body = new QLabel(this);
    m_body->setStyleSheet(QStringLiteral("QLabel { color: #8b8b95; font-size: 12px; }"));
    m_body->setWordWrap(true);
    m_badge = new QLabel(this);
    m_badge->setStyleSheet(QStringLiteral("QLabel { color: #a78bfa; font-size: 11px; letter-spacing: 1px; font-weight: 600; }"));
    textLayout->addWidget(m_title);
    textLayout->addWidget(m_body);
    textLayout->addWidget(m_badge);
    layout->addLayout(textLayout, 1);

    hide();
}

void VerifyOverlay::paintCard(const QColor &accent, const QString &title, const QString &body, const QString &badge)
{
    QPixmap pm = icons::brandPixmap(QStringLiteral(":/branding/paw-icon.svg"), 34, devicePixelRatioF());
    m_icon->setPixmap(pm);
    m_title->setText(title);
    m_title->setStyleSheet(QStringLiteral("QLabel { color: #e4e4e7; font-weight: 600; font-size: 13px; }"));
    m_body->setText(body);
    m_badge->setText(badge);
    m_badge->setStyleSheet(QStringLiteral("QLabel { color: %1; font-size: 11px; letter-spacing: 1px; font-weight: 600; }")
                               .arg(accent.name(QColor::HexRgb)));
    setStyleSheet(styleSheet() + QStringLiteral(
                     // Recompute the border from the accent for a subtle state cue.
                     ""));
    reposition();
    raise();
    show();
}

void VerifyOverlay::showRunning(const QString &issue, int line)
{
    paintCard(QColor(0xa7, 0x8b, 0xfa),
              tr("Editor is not sure about line %1 — checking in the sandbox…").arg(line),
              issue,
              tr("SANDBOX VERIFY · NOTHING IS SAVED"));
}

void VerifyOverlay::showConfirmed(const QString &issue, int line, const QString &detail)
{
    paintCard(QColor(0xf8, 0x71, 0x71),
              tr("Confirmed — this is a real error on line %1").arg(line),
              QStringLiteral("%1%2").arg(issue, detail.isEmpty() ? QString() : QStringLiteral(" · ") + detail),
              tr("SANDBOX RESULT · NOW SHOWN AS AN ERROR"));
}

void VerifyOverlay::showFalseAlarm(const QString &issue, int line)
{
    paintCard(QColor(0x7e, 0xe7, 0x87),
              tr("False alarm — line %1 is fine").arg(line),
              QStringLiteral("%1 · The sandbox found nothing wrong, so the highlight was removed.").arg(issue),
              tr("SANDBOX RESULT · NO ERROR"));
}

void VerifyOverlay::showSureError(const QString &issue, int line)
{
    paintCard(QColor(0xf8, 0x71, 0x71),
              tr("Error on line %1").arg(line),
              QStringLiteral("%1 · The editor is certain, no sandbox check was needed.").arg(issue),
              tr("CONFIRMED BY STATIC ANALYSIS"));
}

void VerifyOverlay::hideSoon(int ms)
{
    QTimer::singleShot(ms, this, [this] { hide(); });
}

void VerifyOverlay::reposition()
{
    QWidget *host = parentWidget();
    if (!host)
        return;
    if (!m_filterInstalled) {
        host->installEventFilter(this);
        m_filterInstalled = true;
    }
    move(host->width() - width() - 28, host->height() - height() - 52);
}

bool VerifyOverlay::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parentWidget() && event->type() == QEvent::Resize)
        reposition();
    return QWidget::eventFilter(watched, event);
}

} // namespace paw
