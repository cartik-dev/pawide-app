#pragma once
#include <QIcon>
#include <QString>

namespace paw::icons {
// Makes sure the Qt resources of the static library are registered (safe to call repeatedly).
void ensureResources();
// Simple stroke icons used by the UI chrome (not part of the brand identity).
QIcon chrome(const QString &name, const QColor &normal, const QColor &active);
// Official brand files from the Qt resource bundle.
QPixmap brandPixmap(const QString &resourcePath, int size, qreal devicePixelRatio = 1.0);
QIcon appIcon();
} // namespace paw::icons
