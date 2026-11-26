#pragma once
#include <QString>

#ifndef PAWIDE_VERSION_STR
#define PAWIDE_VERSION_STR "0.0.0"
#endif

namespace paw {
inline QString appVersion() { return QStringLiteral(PAWIDE_VERSION_STR); }
inline QString appName() { return QStringLiteral("Paw IDE"); }
inline QString repositoryUrl() { return QStringLiteral("https://github.com/cartik-dev/pawide"); }
} // namespace paw
