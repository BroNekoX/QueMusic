#include "AppFont.h"

#include <QFontDatabase>
#include <QGuiApplication>

namespace {

// 平台默认字体。必须在任何 setFont 之前取，否则会把我们设置过的字体当成系统默认。
QFont systemFont()
{
    static const QFont font = QGuiApplication::font();
    return font;
}

QString figtreeFamily()
{
    static const QString family = [] {
        const int id = QFontDatabase::addApplicationFont(
            QStringLiteral(":/QueMusic/resources/fonts/Figtree.otf"));
        if (id < 0)
            return QString();
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        return families.isEmpty() ? QString() : families.first();
    }();
    return family;
}

} // namespace

void AppFont::apply(int mode, const QString &fontStyle)
{
    QFont font = systemFont();
    const QString system = font.family();

    const QString primary = mode == 0 ? figtreeFamily() : mode == 2 ? fontStyle.trimmed() : QString();
    // 字体不存在时退回系统默认，别把界面变成方框
    if (!primary.isEmpty() && primary.compare(system, Qt::CaseInsensitive) != 0
        && QFontDatabase::families().contains(primary, Qt::CaseInsensitive)) {
        // 第一项是主字体，第二项用于缺字回退；这样中文走系统默认字体，
        // 而不是 Qt 自己挑（Windows 上会挑成楷体）。
        font.setFamilies({primary, system});
    }

    QGuiApplication::setFont(font);
}
