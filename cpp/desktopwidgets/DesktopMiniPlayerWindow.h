// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef DESKTOPMINIPLAYERWINDOW_H
#define DESKTOPMINIPLAYERWINDOW_H

#include <QColor>
#include <QImage>
#include <QWidget>

class AudioEngine;
class Playback;
class QPainter;
class QCloseEvent;
class QEnterEvent;
class QMouseEvent;
class QPaintEvent;

// 桌面小窗播放器：由 components/DesktopPlayerWindow.qml 迁移为纯 Widgets 实现，
// 服务于“纯 C++ 常驻层”改造（轻量模式下 QML 引擎被销毁后仍需长期存活）。
// - 数据直连 C++ 单例：Playback / AudioEngine / AppSettings，不依赖任何 QML；
// - 生命周期由 cpp/resident/DesktopWidgetsController 统一管理：开关开→创建
//   显示、开关关→销毁（关闭即销毁，无常驻实例）；本类不连接开关自管显隐，
//   关闭按钮与 closeEvent 仅写回开关，销毁由控制器响应 NOTIFY 完成；
// - 窗口形态与 QML 原版一致：无边框 + 半透明 + 置顶 + Tool（不进任务栏），
//   全卡片拖动、右/底/右下角三向缩放热区；
// - 功能齐全：封面、标题/歌手、可拖动 seek 的进度条与时间标签、循环模式、
//   上一首/播放暂停/下一首、置顶切换（运行时改 WindowStaysOnTopHint）、关闭。
class DesktopMiniPlayerWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DesktopMiniPlayerWindow(QWidget *parent = nullptr);
    ~DesktopMiniPlayerWindow() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    // 控制按钮（对应 QML dpControlRow 的五个 SButton + 右上角 closeButton）
    enum ControlButton {
        ButtonNone,
        ButtonCycle,      // 播放顺序（Playback.cycleIndex +1）
        ButtonPrevious,   // 上一首
        ButtonPlayPause,  // 播放/暂停
        ButtonNext,       // 下一首
        ButtonPinTop,     // 置顶小窗切换
        ButtonClose       // 关闭
    };

    bool isSmallMode() const;  // QML isSmall: height < 132
    QRect coverRect() const;
    QRect sliderRect() const;
    QRect controlButtonRect(ControlButton button) const;
    ControlButton controlButtonAt(const QPoint &pos) const;
    bool resizeEdgesAt(const QPoint &pos, Qt::Edges *edgesOut) const;

    qreal sliderRatio() const;  // 进度条填充比例（拖动中用拖动值）
    void seekToPixel(int x);    // 拖动进度条 → 写回 player 位置（QML onMoved 语义）
    void refreshCoverImage();   // 跟随 player.urlStr 重载封面
    void requestClose();        // 关闭按钮与 closeEvent 的统一出口：写回开关
    void activateButton(ControlButton button);

    void drawCoverImage(QPainter *painter);

    QString currentTimeText() const;  // 已播时间（Playback.fmt）
    QString totalTimeText() const;    // 总时长（Playback.fmt）
    QFont timeLabelFont() const;      // 时间标签字体（11px，Theme.textColor）
    static QFont iconFont(const QString &family, int pixelSize);

    Playback *m_playback = nullptr;   // Playback 单例（不拥有）
    AudioEngine *m_player = nullptr;  // m_playback->player()（不拥有）
    QString m_iconFamily;             // feather 图标字体族名
    QImage m_coverImage;              // 当前封面（跟随 player.urlStr 刷新）
    bool m_topWindow = true;          // 置顶状态（QML topWindow 初始 true）
    bool m_seekDragging = false;      // 进度条拖动中（QML Slider pressed）
    qint64 m_seekValue = 0;           // 拖动中的目标位置（毫秒）
    ControlButton m_pressedButton = ButtonNone;
    ControlButton m_hoveredButton = ButtonNone;
};

#endif // DESKTOPMINIPLAYERWINDOW_H
