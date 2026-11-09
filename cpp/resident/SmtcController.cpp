// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "SmtcController.h"

#include "QueueModel.h"
#include "WindowsSmtcManager.h"
#include "audio/AudioEngine.h"
#include "playback/Playback.h"

#include <QDateTime>
#include <QTimer>

// 宿主窗口注册（HWND）只对 Windows 有意义；非 Windows 平台整体为空操作，
// 等价原 QML 桥接在 mgr.available 恒 false 时完全不推送的行为。
#ifdef Q_OS_WIN

// 对应原 QML 桥接的 queued/infoQueued：同帧内的多次触发合并为一次。
// QML 用 Qt.callLater 延后；C++ 用零延迟单发定时器（事件循环空闲时执行）等价。
namespace {
constexpr int kMergeDelayMs = 0;
// 时间线节流阈值（原 onPositionChanged）：5 秒推送周期 / 3 秒位置跳变
constexpr qint64 kTimelineIntervalMs = 5000;
constexpr qint64 kTimelineJumpMs = 3000;
} // namespace

SmtcController::SmtcController(Playback *playback, QObject *parent)
    : QObject(parent)
    , m_playback(playback)
{
    if (!m_playback)
        return;

    // 隐藏宿主窗口：SMTC 经 GetForWindow 以 HWND 注册，不再依赖主窗口——
    // 轻量模式下主窗口销毁后系统媒体控件仍随常驻层存活。
    m_hostWindow = new QWindow();
    m_hostWindow->create();   // 强制创建 native 平台窗口（不 show，保持隐藏）

    m_mgr = new WindowsSmtcManager(this);

    // 系统按钮/时间线拖动回发 → 播放控制。原生 WinRT 调用会同步回调进本管线
    // （重入切歌逻辑），故统一 Qt::QueuedConnection 延后到事件循环
    // （等价原 QML 注释"统一经 Qt.callLater 延后，避免重入"）。
    connect(m_mgr, &WindowsSmtcManager::playPressed, this,
            [this] { m_playback->togglePlay(); }, Qt::QueuedConnection);
    connect(m_mgr, &WindowsSmtcManager::pausePressed, this,
            [this] { m_playback->togglePlay(); }, Qt::QueuedConnection);
    connect(m_mgr, &WindowsSmtcManager::nextPressed, this,
            [this] { m_playback->next(false); }, Qt::QueuedConnection);
    connect(m_mgr, &WindowsSmtcManager::previousPressed, this,
            [this] { m_playback->previous(); }, Qt::QueuedConnection);
    connect(m_mgr, &WindowsSmtcManager::seekRequested, this,
            [this](qint64 pos) { m_playback->player()->setPosition(pos); },
            Qt::QueuedConnection);

    // AudioEngine 变化（原 playerWatch Connections，target: Playback.player）
    AudioEngine *player = m_playback->player();
    if (player) {
        connect(player, &AudioEngine::sourceChanged, this, [this] {
            m_playback->clearAb();   // 换曲时清除 A-B 循环（原 onSourceChanged 逻辑）
            push();
        });
        connect(player, &AudioEngine::durationChanged, this, [this] { push(); });
        connect(player, &AudioEngine::metaDataChanged, this, [this] { pushInfo(); });
        connect(player, &AudioEngine::playbackStateChanged, this, [this] { push(); });
        connect(player, &AudioEngine::positionChanged, this, [this] { updateTimeline(); });
    }

    // 曲目信息/播放状态变化（原 metaWatch Connections，target: Playback）
    connect(m_playback, &Playback::musicTitleChanged, this, [this] { pushInfo(); });
    connect(m_playback, &Playback::musicArtistChanged, this, [this] { pushInfo(); });
    connect(m_playback, &Playback::inplayChanged, this, [this] { push(); });

    // 队列变化（原 queueWatch Connections，target: Playback.queue）
    QueueModel *queue = m_playback->queue();
    if (queue) {
        connect(queue, &QueueModel::countChanged, this, [this] { push(); });
        connect(queue, &QueueModel::playListIndexChanged, this, [this] { push(); });
    }

    // 原 mgr 的 Component.onCompleted：initialize(targetWindow) + updateControls()
    m_mgr->initialize(m_hostWindow);
    updateControls();
}

SmtcController::~SmtcController()
{
    if (m_mgr) {
        m_mgr->shutdown();   // 置 Closed / 解绑 WinRT 事件 / 清静态回调指针
        delete m_mgr;
        m_mgr = nullptr;
    }
    delete m_hostWindow;
    m_hostWindow = nullptr;
}

// 播放状态/时间线整帧推送（原 push()）：inplay 优先 Playing；否则引擎处于
// Playing/Paused 态按 Paused 推（缓冲/换曲瞬间系统按钮仍显示"暂停"）；再否则
// Stopped；最后 Closed。duration>0 时顺带把时间线归零推送（新曲起点）。
void SmtcController::push()
{
    if (m_queued)
        return;
    m_queued = true;
    QTimer::singleShot(kMergeDelayMs, this, [this] {
        m_queued = false;
        updateControls();
        if (!m_mgr->isAvailable())
            return;
        AudioEngine *player = m_playback->player();
        if (player->duration() > 0)
            m_mgr->updateTimeline(0, player->duration());
        if (m_playback->inplay())
            m_mgr->setPlaybackStatus(WindowsSmtcManager::Playing);
        else if (player->playbackState() == AudioEngine::PlayingState
                 || player->playbackState() == AudioEngine::PausedState)
            m_mgr->setPlaybackStatus(WindowsSmtcManager::Paused);
        else if (player->playbackState() == AudioEngine::StoppedState)
            m_mgr->setPlaybackStatus(WindowsSmtcManager::Stopped);
        else
            m_mgr->setPlaybackStatus(WindowsSmtcManager::Closed);
    });
}

// 曲目元信息推送（原 pushInfo()）：title/artist 来自 Playback（与托盘一致），
// album/封面 url 来自 AudioEngine 元数据；mediaId 取队列当前项 path（原 QML
// 用 queue.get(idx).path，此处 trackAt 等价，另补上界保护）。
void SmtcController::pushInfo()
{
    if (m_infoQueued)
        return;
    m_infoQueued = true;
    QTimer::singleShot(kMergeDelayMs, this, [this] {
        m_infoQueued = false;
        if (!m_mgr->isAvailable())
            return;
        AudioEngine *player = m_playback->player();
        QueueModel *queue = m_playback->queue();
        QString mediaId;
        const int index = queue ? queue->playListIndex() : -1;
        if (queue && queue->count() > 0 && index >= 0 && index < queue->count())
            mediaId = queue->trackAt(index).path;
        m_mgr->updateMediaInfo(m_playback->musicTitle(), m_playback->musicArtist(),
                               player->albumTitle(), player->urlStr(), mediaId);
    });
}

// prev/next 可用性（原 updateControls()）：队列首尾判断，同帧 push 时先于
// 状态推送执行（保持原顺序）。
void SmtcController::updateControls()
{
    if (!m_mgr->isAvailable())
        return;
    QueueModel *queue = m_playback->queue();
    if (!queue)
        return;
    m_mgr->setControlsEnabled(true, true,
                              queue->playListIndex() < queue->count() - 1,
                              queue->playListIndex() > 0);
}

// 播放位置推送（原 onPositionChanged 节流）：距上次推送不足 5 秒且位置跳变小
// （<3 秒）且距结尾还有 5 秒以上 → 忽略；其余（5 秒周期到点 / 拖动跳变 /
// 接近结尾补推）→ updateTimeline。
void SmtcController::updateTimeline()
{
    if (!m_mgr->isAvailable())
        return;
    AudioEngine *player = m_playback->player();
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    const qint64 jump = qAbs(player->position() - m_lastPositionMs);
    if (now - m_lastTimelineMs < kTimelineIntervalMs
            && jump < kTimelineJumpMs
            && player->duration() - player->position() > kTimelineIntervalMs)
        return;
    m_lastTimelineMs = now;
    m_lastPositionMs = player->position();
    m_mgr->updateTimeline(player->position(), player->duration());
}

#else // 非 Windows：整体空操作

SmtcController::SmtcController(Playback *playback, QObject *parent)
    : QObject(parent)
    , m_playback(playback)
{
}

SmtcController::~SmtcController() = default;

void SmtcController::push() {}
void SmtcController::pushInfo() {}
void SmtcController::updateControls() {}
void SmtcController::updateTimeline() {}

#endif // Q_OS_WIN
