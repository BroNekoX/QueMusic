#pragma once

#include "AppSettings.h"

#include <QColor>
#include <QList>
#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

class Style final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    static Style *instance();
    static Style *create(QQmlEngine *, QJSEngine *);

    static QList<QColor> defaultColorList();

// 预处理设置项，让他编译生成设置项内容
#define QUE_MUSIC_STYLE(set) \
    set(int, theme, 0) \
    set(int, color, 0) \
    set(QList<QColor>, colorList, defaultColorList()) \
    set(int, backmode, 0) \
    set(int, backpic, 0) \
    set(int, colorStyle, 0) \
    set(qreal, shadowBlur, 1.0) \
    set(int, shadowXOffset, 5) \
    set(int, shadowYOffset, 5) \
    set(int, shadowSize, 16) \
    set(int, blurSize, 48) \
    set(bool, highQualityBlur, false) \
    set(int, textH1, 22) \
    set(int, textH2, 17) \
    set(int, textmain, 13) \
    set(int, text, 12) \
    set(int, textTip, 11) \
    set(int, texticon, 16) \
    set(int, pageTitle, 26) \
    set(int, texticonH, 22) \
    set(int, labelRadius, 12) \
    set(int, cubeRadius, 16) \
    set(bool, noControlRadius, false) \
    set(qreal, borderDepth, 0.08) \
    set(bool, layerEnabled, true) \
    set(bool, premiumAnime, false) \
    set(int, animeSpeed, 1) \
    set(int, homeLayout, 0) \
    set(bool, noOpacityControl, false) \
    set(bool, coverColorControl, false) \
    set(int, leftBarMode, 0) \
    set(bool, sidebarColor, true) \
    set(int, sidebarStyle, 1) \
    set(int, animeType, 0) \
    set(int, menutheme, 1) \
    set(int, glmode, 0) \
    set(int, uilevel, 0) \
    set(int, lyricSize, 10) \
    set(int, flowStyle, 1) \
    set(bool, waveDisplay, true) \
    set(bool, premiumLyricAnime, true) \
    set(int, textWidth, 600) \
    set(bool, maskBlur, true) \
    set(bool, lyricHideGui, true) \
    set(int, springValue, 2) \
    set(bool, lyricBasicCd, false) \
    set(QString, fontFamily, QString()) \
    set(QString, backgroundImage, QStringLiteral("qrc:/QueMusic/resources/pic/back2.jpg"))

#define QUE_MUSIC_STYLE_PROPERTY(TYPE, NAME, DEFAULT) \
    Q_PROPERTY(TYPE NAME READ NAME WRITE set_##NAME NOTIFY NAME##Changed FINAL) \
public: \
    TYPE NAME() const { return m_##NAME; } \
    void set_##NAME(const TYPE &value) \
    { \
        if (m_##NAME == value) \
            return; \
        m_##NAME = value; \
        AppSettings::set(QStringLiteral("Style"), #NAME, value); \
        emit NAME##Changed(); \
    } \
private: \
    TYPE m_##NAME = DEFAULT;

    QUE_MUSIC_STYLE(QUE_MUSIC_STYLE_PROPERTY)
#undef QUE_MUSIC_STYLE_PROPERTY

signals:
#define QUE_MUSIC_STYLE_SIGNAL(TYPE, NAME, DEFAULT) void NAME##Changed();
    QUE_MUSIC_STYLE(QUE_MUSIC_STYLE_SIGNAL)
#undef QUE_MUSIC_STYLE_SIGNAL

private:
    explicit Style(QObject *parent = nullptr);
};
