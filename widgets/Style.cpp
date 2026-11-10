#include "Style.h"

#include <QQmlEngine>

Style *Style::instance()
{
    static Style *instance = nullptr;
    if (!instance) {
        instance = new Style();
        QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
    }
    return instance;
}

Style *Style::create(QQmlEngine *, QJSEngine *)
{
    return instance();
}

QList<QColor> Style::defaultColorList()
{
    return {QColor(QStringLiteral("#3481fa")), QColor(QStringLiteral("#34fa4a")),
            QColor(QStringLiteral("#faad34")), QColor(QStringLiteral("#ad34fa"))};
}

Style::Style(QObject *parent)
    : QObject(parent)
{
#define QUE_MUSIC_STYLE_LOAD(TYPE, NAME, DEFAULT) m_##NAME = AppSettings::get<TYPE>(QStringLiteral("Style"), #NAME, DEFAULT);
    QUE_MUSIC_STYLE(QUE_MUSIC_STYLE_LOAD)
#undef QUE_MUSIC_STYLE_LOAD
}
