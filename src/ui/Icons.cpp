#include "Icons.h"

#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QSvgRenderer>

// Resources live in a static library, so the linker would drop them unless initialised explicitly.
// Q_INIT_RESOURCE must be used outside of any namespace.
static void pawInitResources() { Q_INIT_RESOURCE(resources); }

namespace paw::icons {

void ensureResources()
{
    static const bool done = (pawInitResources(), true);
    Q_UNUSED(done)
}

static const QHash<QString, QString> &shapes()
{
    static const QHash<QString, QString> m = {
        {"explorer", "<path d='M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z'/><path d='M14 3v5h5'/>"},
        {"search", "<circle cx='11' cy='11' r='7'/><path d='M21 21l-4.3-4.3'/>"},
        {"git", "<circle cx='6' cy='5' r='2.2'/><circle cx='6' cy='19' r='2.2'/><circle cx='18' cy='8' r='2.2'/><path d='M6 7.2v9.6M18 10.2c0 5-12 2-12 6.6'/>"},
        {"run", "<path d='M7 4l13 8-13 8z'/>"},
        {"extensions", "<rect x='3' y='13' width='8' height='8' rx='1'/><rect x='13' y='13' width='8' height='8' rx='1'/><rect x='3' y='3' width='8' height='8' rx='1'/><path d='M17 2.5v7M13.5 6h7'/>"},
        {"settings", "<circle cx='12' cy='12' r='3'/><path d='M12 2v3M12 19v3M2 12h3M19 12h3M4.9 4.9l2.1 2.1M17 17l2.1 2.1M4.9 19.1L7 17M17 7l2.1-2.1'/>"},
        {"newfile", "<path d='M14 3H7a2 2 0 0 0-2 2v14a2 2 0 0 0 2 2h10a2 2 0 0 0 2-2V8z'/><path d='M14 3v5h5M12 12v6M9 15h6'/>"},
        {"newfolder", "<path d='M3 6a2 2 0 0 1 2-2h4l2 2h8a2 2 0 0 1 2 2v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z'/><path d='M12 10v6M9 13h6'/>"},
        {"refresh", "<path d='M20 11a8 8 0 1 0-2.3 5.7'/><path d='M20 4v7h-7'/>"},
        {"collapse", "<path d='M8 4l4 4 4-4M8 20l4-4 4 4M4 12h16'/>"},
        {"close", "<path d='M6 6l12 12M18 6L6 18'/>"},
        {"play", "<path d='M7 4l13 8-13 8z'/>"},
        {"bug", "<circle cx='12' cy='13' r='5'/><path d='M12 8V5M9.5 5.5L8 3.5M14.5 5.5L16 3.5M7 11H4M7 15H4.5M20 11h-3M19.5 15H17M9 13h.01M15 13h.01'/>"},
        {"stop", "<rect x='6' y='6' width='12' height='12' rx='1.5'/>"},
        {"plus", "<path d='M12 5v14M5 12h14'/>"},
        {"check", "<path d='M4.5 12.5l5 5 10-11'/>"},
        {"shield", "<path d='M12 3l8 3v6c0 5-3.5 8-8 9-4.5-1-8-4-8-9V6z'/><path d='M9 12l2 2 4-4'/>"},
    };
    return m;
}

static QPixmap render(const QString &name, const QColor &color, int size, qreal dpr)
{
    const QString svg = QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='%1' "
                                       "stroke-width='1.7' stroke-linecap='round' stroke-linejoin='round'>%2</svg>")
                            .arg(color.name(QColor::HexRgb), shapes().value(name));
    QSvgRenderer r(svg.toUtf8());
    QPixmap pm(QSize(size, size) * dpr);
    pm.setDevicePixelRatio(dpr);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    r.render(&p, QRectF(0, 0, size, size));
    return pm;
}

QIcon chrome(const QString &name, const QColor &normal, const QColor &active)
{
    QIcon icon;
    for (int s : {16, 20, 24, 32}) {
        icon.addPixmap(render(name, normal, s, 2.0), QIcon::Normal, QIcon::Off);
        icon.addPixmap(render(name, active, s, 2.0), QIcon::Normal, QIcon::On);
        icon.addPixmap(render(name, active, s, 2.0), QIcon::Active, QIcon::Off);
    }
    return icon;
}

QPixmap brandPixmap(const QString &resourcePath, int size, qreal dpr)
{
    ensureResources();
    if (resourcePath.endsWith(QLatin1String(".svg"))) {
        QSvgRenderer r(resourcePath);
        QPixmap pm(QSize(size, size) * dpr);
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        r.render(&p, QRectF(0, 0, size, size));
        return pm;
    }
    QPixmap pm(resourcePath);
    return pm.scaled(QSize(size, size) * dpr, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

QIcon appIcon()
{
    ensureResources();
    QIcon icon;
    for (int s : {16, 24, 32, 48, 64, 128, 256})
        icon.addPixmap(brandPixmap(QStringLiteral(":/branding/paw-icon.svg"), s, 1.0));
    return icon;
}

} // namespace paw::icons
