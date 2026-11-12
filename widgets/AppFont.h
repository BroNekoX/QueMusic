#pragma once

#include <QString>

// 全局应用字体。mode: 0 = Figtree（英文；中文等缺字由 Qt 按字符回退到系统字体）
//                      1 = 系统默认
//                      2 = fontStyle 指定的字体
namespace AppFont {

// 写进 QGuiApplication。只影响之后创建的 QML Text，已有 Text 由 App 刷新。
void apply(int mode, const QString &fontStyle);

}