#pragma once
#include <QColor>
#include <QString>

class QApplication;

namespace paw::theme {
// Black / white / violet identity of Paw IDE.
inline const QColor bgWindow{0x0b, 0x0b, 0x0d};
inline const QColor bgActivity{0x08, 0x08, 0x0a};
inline const QColor bgSideBar{0x10, 0x10, 0x13};
inline const QColor bgEditor{0x0c, 0x0c, 0x0f};
inline const QColor bgLineHighlight{0x15, 0x15, 0x1b};
inline const QColor border{0x22, 0x22, 0x29};
inline const QColor text{0xe4, 0xe4, 0xe7};
inline const QColor textMuted{0x8b, 0x8b, 0x95};
inline const QColor accent{0x8b, 0x5c, 0xf6};
inline const QColor accentLight{0xa7, 0x8b, 0xfa};
inline const QColor selection{0x3b, 0x2a, 0x6b};

QString styleSheet();
void apply(QApplication &app);
} // namespace paw::theme
