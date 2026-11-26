// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
#include "MacMediaSessionBackend.h"

#include <QtCore/QMetaObject>
#include <QtCore/QString>
#include <QtCore/QUrl>

#import <AppKit/AppKit.h>
#import <MediaPlayer/MediaPlayer.h>

namespace {

void QMMacApplyNowPlaying(const MediaTrackInfo &info, qint64 positionMs, bool playing)
{
    MPNowPlayingInfoCenter *center = MPNowPlayingInfoCenter.defaultCenter;
    NSMutableDictionary *items = [NSMutableDictionary dictionary];

    if (!info.title.isEmpty())
        items[MPMediaItemPropertyTitle] = info.title.toNSString();
    if (!info.artist.isEmpty())
        items[MPMediaItemPropertyArtist] = info.artist.toNSString();
    if (!info.album.isEmpty())
        items[MPMediaItemPropertyAlbumTitle] = info.album.toNSString();
    if (info.durationMs > 0)
        items[MPMediaItemPropertyPlaybackDuration] = @(info.durationMs / 1000.0);

    items[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(positionMs / 1000.0);
    items[MPNowPlayingInfoPropertyPlaybackRate] = playing ? @1.0 : @0.0;

    // 只处理本地封面；网络封面交给系统缓存不划算，保持默认图标
    if (info.cover.isLocalFile()) {
        NSImage *image = [[NSImage alloc] initWithContentsOfFile:info.cover.toLocalFile().toNSString()];
        if (image)
            items[MPMediaItemPropertyArtwork] = [[MPMediaItemArtwork alloc] initWithImage:image];
    }

    center.nowPlayingInfo = items;
}

} // namespace

// 远程命令目标由单例持有：会话对象重建时无需重复注册
@interface QMMacNowPlaying : NSObject
@property (nonatomic, strong) NSMutableArray *targets;
@property (nonatomic, assign) void *backend;
@property (nonatomic, assign) BOOL active;
+ (instancetype)shared;
- (BOOL)activate;
- (void)deactivate;
@end

@implementation QMMacNowPlaying

+ (instancetype)shared
{
    static QMMacNowPlaying *session = nil;
    if (!session)
        session = [[QMMacNowPlaying alloc] init];
    return session;
}

- (instancetype)init
{
    self = [super init];
    if (self) {
        _targets = [NSMutableArray array];
        _active = NO;
    }
    return self;
}

- (void)addHandler:(MPRemoteCommand *)command action:(void (^)(void))action
{
    if (!command)
        return;
    id token = [command addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
        Q_UNUSED(event);
        action();
        return MPRemoteCommandHandlerStatusSuccess;
    }];
    if (token)
        [self.targets addObject:@[command, token]];
}

- (void)removeAllTargets
{
    for (NSArray *pair in self.targets) {
        if (pair.count == 2)
            [(MPRemoteCommand *)pair[0] removeTarget:pair[1]];
    }
    [self.targets removeAllObjects];
}

- (BOOL)activate
{
    if (self.active)
        return YES;

    MPRemoteCommandCenter *center = MPRemoteCommandCenter.sharedCommandCenter;
    if (!center)
        return NO;

    void *backend = self.backend;
    const auto send = [backend](const char *member) {
        if (backend)
            QMetaObject::invokeMethod(static_cast<QObject *>(backend), member, Qt::QueuedConnection);
    };

    center.playCommand.enabled = YES;
    center.pauseCommand.enabled = YES;
    center.togglePlayPauseCommand.enabled = YES;
    center.stopCommand.enabled = YES;
    center.nextTrackCommand.enabled = YES;
    center.previousTrackCommand.enabled = YES;
    center.changePlaybackPositionCommand.enabled = YES;

    [self addHandler:center.playCommand action:^{ send("playRequested"); }];
    [self addHandler:center.pauseCommand action:^{ send("pauseRequested"); }];
    [self addHandler:center.togglePlayPauseCommand action:^{ send("togglePlayPauseRequested"); }];
    [self addHandler:center.stopCommand action:^{ send("stopRequested"); }];
    [self addHandler:center.nextTrackCommand action:^{ send("nextRequested"); }];
    [self addHandler:center.previousTrackCommand action:^{ send("previousRequested"); }];

    id token = [center.changePlaybackPositionCommand
        addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
            MPChangePlaybackPositionCommandEvent *seekEvent =
                (MPChangePlaybackPositionCommandEvent *)event;
            if (backend) {
                QMetaObject::invokeMethod(static_cast<QObject *>(backend), "seekRequested",
                                          Qt::QueuedConnection,
                                          Q_ARG(qint64, qint64(seekEvent.positionTime * 1000.0)));
            }
            return MPRemoteCommandHandlerStatusSuccess;
        }];
    if (token)
        [self.targets addObject:@[center.changePlaybackPositionCommand, token]];

    self.active = YES;
    return YES;
}

- (void)deactivate
{
    if (!self.active)
        return;
    [self removeAllTargets];
    self.active = NO;
}

@end

MacMediaSessionBackend::MacMediaSessionBackend(QObject *parent)
    : MediaSessionBackend(parent)
{
    QMMacNowPlaying.shared.backend = this;
}

MacMediaSessionBackend::~MacMediaSessionBackend()
{
    shutdown();
}

bool MacMediaSessionBackend::available() const
{
    return m_active;
}

void MacMediaSessionBackend::initialize(QWindow *)
{
    if (m_active)
        return;
    QMMacNowPlaying *session = QMMacNowPlaying.shared;
    session.backend = this;
    m_active = [session activate] == YES;
    emit availableChanged();
    publish();
}

void MacMediaSessionBackend::shutdown()
{
    if (!m_active)
        return;
    [QMMacNowPlaying.shared deactivate];
    QMMacNowPlaying.shared.backend = nullptr;
    MPNowPlayingInfoCenter.defaultCenter.nowPlayingInfo = nil;
    MPNowPlayingInfoCenter.defaultCenter.playbackState = MPNowPlayingPlaybackStateStopped;
    m_active = false;
    emit availableChanged();
}

void MacMediaSessionBackend::setStatus(PlaybackStatus status)
{
    if (m_status == status)
        return;
    m_status = status;
    publish();
}

void MacMediaSessionBackend::setTrack(const MediaTrackInfo &info)
{
    m_track = info;
    publish();
}

void MacMediaSessionBackend::setPosition(qint64 positionMs, qint64 durationMs)
{
    m_positionMs = positionMs;
    if (durationMs > 0)
        m_track.durationMs = durationMs;
    publish();
}

void MacMediaSessionBackend::publish()
{
    if (!m_active)
        return;
    const bool playing = m_status == Playing;
    QMMacApplyNowPlaying(m_track, m_positionMs, playing);
    MPNowPlayingInfoCenter.defaultCenter.playbackState =
        playing ? MPNowPlayingPlaybackStatePlaying
                : (m_status == Paused ? MPNowPlayingPlaybackStatePaused
                                      : MPNowPlayingPlaybackStateStopped);
}
