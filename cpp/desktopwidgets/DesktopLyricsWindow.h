// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef DESKTOPLYRICSWINDOW_H
#define DESKTOPLYRICSWINDOW_H

#include <QColor>
#include <QWidget>

class AudioEngine;
class Playback;
class QPainter;
class QCloseEvent;
class QEnterEvent;
class QHideEvent;
class QMouseEvent;
class QPaintEvent;
class QShowEvent;
class QTimer;

// 桌面歌词悬浮窗：由 components/DesktopLyrics.qml 迁移为纯 Widgets 实现，
// 服务于“纯 C++ 常驻层”改造（轻量模式下 QML 引擎被销毁后仍需长期存活）。
// - 数据直连 C++ 单例：Playback / AudioEngine / MusicApiService / AppSettings，
//   不依赖任何 QML；
// - 生命周期由 cpp/resident/DesktopWidgetsController 统一管理：开关开→创建
//   显示、开关关→销毁（关闭即销毁，无常驻实例）；本类不连接开关自管显隐，
//   关闭按钮与 closeEvent 仅写回开关，销毁由控制器响应 NOTIFY 完成；
// - 窗口形态与 QML 原版一致：无边框 + 半透明 + 置顶 + Tool（不进任务栏），
//   全窗拖动、五向缩放热区、悬停工具条（歌名-歌手 / 上一首 / 播放暂停 /
//   下一首 / 已播-总长 / 关闭）。
class DesktopLyricsWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DesktopLyricsWindow(QWidget *parent = nullptr);
    ~DesktopLyricsWindow() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    // 悬浮工具条上的可点击按钮（对应 QML 内的四个 SButton）
    enum ToolbarButton {
        ButtonNone,
        ButtonPrevious,
        ButtonPlayPause,
        ButtonNext,
        ButtonClose
    };

    void updateLyricIndex();   // QML updateCurrentIndex() 的移植（增量推进歌词下标）
    void updateTickTimer();    // QML Timer.running = onMedia && visible 的移植
    void requestClose();       // 关闭按钮与 closeEvent 的统一出口：写回开关
    void activateToolbarButton(ToolbarButton button);

    QRect toolbarButtonRect(ToolbarButton button) const;
    ToolbarButton toolbarButtonAt(const QPoint &pos) const;
    bool resizeEdgesAt(const QPoint &pos, Qt::Edges *edgesOut) const;

    void drawShadowedText(QPainter *painter, const QRect &rect, const QFont &font,
                          const QColor &color, const QString &text,
                          const QColor &shadowColor, int shadowOffset) const;
    void drawToolbarButton(QPainter *painter, const QRect &rect, const QString &glyph,
                           int iconSize, bool hovered) const;

    Playback *m_playback = nullptr;   // Playback 单例（不拥有）
    AudioEngine *m_player = nullptr;  // m_playback->player()（不拥有）
    QTimer *m_tickTimer = nullptr;    // 歌词下标推进计时器（240ms）
    QString m_iconFamily;             // feather 图标字体族名
    int m_currentIndex = 0;           // 当前行下标（-1 表示无歌词）
    int m_nextIndex = 1;              // 下一行下标（-1 表示无歌词）
    bool m_hovered = false;           // 鼠标悬停 → 工具条显隐（Widgets 版即时切换）
    ToolbarButton m_pressedButton = ButtonNone;
    ToolbarButton m_hoveredButton = ButtonNone;
};

#endif // DESKTOPLYRICSWINDOW_H
