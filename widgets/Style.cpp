#include "Style.h"

#include "AppFont.h"

#include <QQmlEngine>
#include <QMetaType>

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
    // QSettings 把 colorList 存成 @Variant(QList<QColor>) 二进制，读之前必须让
    // 元类型系统认识这个名字，否则 QVariant 反序列化失败、列表变空。
    qRegisterMetaType<QList<QColor>>("QList<QColor>");

#define QUE_MUSIC_STYLE_LOAD(TYPE, NAME, DEFAULT) m_##NAME = AppSettings::get<TYPE>(QStringLiteral("Style"), #NAME, DEFAULT);
    QUE_MUSIC_STYLE(QUE_MUSIC_STYLE_LOAD)
#undef QUE_MUSIC_STYLE_LOAD

    if (m_colorList.isEmpty())
        m_colorList = defaultColorList();
    if (m_color >= m_colorList.size())
        m_color = 0;

    applyGlobalFont();
}

void Style::applyGlobalFont()
{
    AppFont::apply(m_globalFont, m_fontStyle);
}
