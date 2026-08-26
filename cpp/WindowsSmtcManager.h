// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#ifndef WINDOWSSMTCMANAGER_H
#define WINDOWSSMTCMANAGER_H

#include <QObject>
#include <QString>
#include <QWindow>
#include <QtQmlIntegration/qqmlintegration.h>

// Windows SMTC（System Media Transport Controls，系统媒体传输控件）管理器。
// 在 Windows 下启用（MSVC 与 MinGW-w64 均可，直连 WinRT ABI，不依赖 WRL）；
// 其他平台保留空操作接口，QML 侧可直接使用。
class WindowsSmtcManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    // 是否已成功初始化（在 Windows 下才可能为 true）
    Q_PROPERTY(bool available READ isAvailable NOTIFY availableChanged)

public:
    // 与 Windows.Media.MediaPlaybackStatus 一一对应，供 QML 直接使用
    enum PlaybackStatus {
        Closed = 0,
        Changing = 1,
        Stopped = 2,
        Playing = 3,
        Paused = 4
    };
    Q_ENUM(PlaybackStatus)

    explicit WindowsSmtcManager(QObject *parent = nullptr);
    ~WindowsSmtcManager() override;

    bool isAvailable() const;

    // 绑定到主窗口（需要在窗口显示后调用；内部会确保 native window 已创建）
    Q_INVOKABLE void initialize(QWindow *window);
    Q_INVOKABLE void shutdown();

    // 启用/禁用 SMTC 控制按钮（Play/Pause/Next/Previous）
    Q_INVOKABLE void setControlsEnabled(bool play, bool pause, bool next, bool previous);
    Q_INVOKABLE void setPlaybackStatus(int status);
    Q_INVOKABLE void updateMediaInfo(const QString &title, const QString &artist,
                                     const QString &album = QString());
    // position/duration 单位为毫秒；每 5 秒调用一次即可，切歌/暂停时也建议调用
    Q_INVOKABLE void updateTimeline(qint64 positionMs, qint64 durationMs);

    // 内部使用：SMTC 按钮事件回发的线程安全入口（可能由 WinRT 工作线程调用）
    void postButtonPressed(int button);
    void postSeekRequested(qint64 positionMs);

signals:
    void availableChanged();
    void playPressed();
    void pausePressed();
    void nextPressed();
    void previousPressed();
    // SMTC 时间线拖动（由 SystemMediaTransportControls2 的 PositionChangeRequested 触发）
    void seekRequested(qint64 positionMs);

private:
    class Private;
    Private *d = nullptr;
};

#endif // WINDOWSSMTCMANAGER_H