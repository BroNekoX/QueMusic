// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
//

pragma ComponentBehavior: Bound
import QtQuick
import QueMusic 1.0

QScrollView {
    id: page
    property int standWidth: 0
    property int containWidth: 0
    property int containX: 0
    property color cardColor: Options.cardColor

    y: 60
    width: parent ? parent.width : 0
    height: parent ? parent.height - 60 : 0
    visible: false

    Column {
        spacing: 16
        padding: 24
        width: page.containWidth
        x: page.containX

        Text {
            height: 40
            color: Theme.fontColor
            verticalAlignment: Text.AlignVCenter
            text: "播放器"
            font.pixelSize: Style.pageTitle
            font.weight: Font.DemiBold
        }

        QHead { text: "在线功能" }

        Rectangle {
            width: page.standWidth
            color: page.cardColor
            radius: Style.cubeRadius
            Column {
                width: parent.width
                padding: 0
                Component.onCompleted: parent.height = height

                SettingItemCard {
                    label: "默认音质"
                    tip: "设置在线音乐默认播放的音质等级"
                    QDrop {
                        choice: Config.soundQuality
                        model: ["标准-128k","高清-320k","无损-500+k"]
                        onTransformed: (choiced) => {
                            Config.soundQuality = choiced
                        }
                    }
                }

                SettingItemCard {
                    label: "默认音乐源"
                    tip: "设置在线音乐默认使用的音源平台"
                    QDrop {
                        choice: Math.max(0, MusicApi.sourceIds.indexOf(Config.mainMusicSource))
                        model: MusicApi.sourceNames
                        onTransformed: (choiced) => {
                            Config.mainMusicSource = MusicApi.sourceIdAt(choiced)
                            MusicApi.songSource = Config.mainMusicSource
                        }
                    }
                }

                SettingItemCard {
                    label: "缓存大小/MB"
                    tip: "设置图片缓存的磁盘占用上限，超出后自动清理"
                    QSlider {
                        from: 200
                        to: 1000
                        stepSize: 10
                        leftText: true
                        valueText: value
                        value: Config.cacheSize
                        onMoved: {
                            Config.cacheSize = value
                            Playback.coverHelper.pruneCache(value)
                        }
                    }
                    bottomLine: false
                }
            }
        }

        QHead { text: "通用" }

        Rectangle {
            width: page.standWidth
            color: page.cardColor
            radius: Style.cubeRadius
            height: toGenCol.height
            Column {
                id: toGenCol
                width: parent.width
                padding: 0

                SettingItemCard {
                    label: "使用默认输出设备"
                    tip: "跟随系统当前默认的音频输出设备播放"
                    QSwitch {
                        letRight: true
                        switchTrue: Playback.player.followSystemDevice
                        onToggled: Playback.player.followSystemDevice = !Playback.player.followSystemDevice
                    }
                }

                SettingsItemVisibleCard {
                    label: "音频输出设备"
                    tip: "指定播放时使用的音频输出设备"
                    display: !Playback.player.followSystemDevice
                    Behavior on height { NumberAnimation { duration: 320; easing.type: Easing.OutExpo } }
                    QDrop {
                        choice: Playback.player.deviceIndex
                        model: Playback.player.audioDevices
                        onTransformed: (choiced) => {
                            Playback.player.deviceIndex = choiced
                        }
                    }
                }

                SettingItemCard {
                    label: "自动播放"
                    tip: "启动应用后自动开始播放"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.autoPlay
                        onToggled: Config.autoPlay = !Config.autoPlay
                    }
                }

                SettingItemCard {
                    label: "扫描子目录"
                    tip: "本地文件夹中将显示子目录下的歌曲"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.recursive
                        onToggled: Config.recursive = !Config.recursive
                    }
                }

                SettingItemCard {
                    label: "音量步长"
                    tip: "每次滚轮或快捷键调节音量的幅度"
                    QSlider {
                        from: 1
                        to: 20
                        stepSize: 1
                        leftText: true
                        valueText: value.toString() + " %"
                        value: Config.volumeStep
                        onMoved: {
                            Config.volumeStep = value
                        }
                    }
                    bottomLine: false
                }
            }
        }

        QHead { text: "播放控制" }

        Rectangle {
            width: page.standWidth
            color: page.cardColor
            radius: Style.cubeRadius
            Column {
                width: parent.width
                padding: 0
                Component.onCompleted: parent.height = height

                SettingItemCard {
                    label: "精确跳转步长"
                    tip: "快捷键快进/快退一次跳转的秒数"
                    QDrop {
                        choice: [3,5,10,15,30].indexOf(Config.seekStep)
                        model: ["3 秒","5 秒","10 秒","15 秒","30 秒"]
                        onTransformed: (choiced) => Config.seekStep = [3,5,10,15,30][choiced]
                    }
                }

                SettingItemCard {
                    label: "播放淡入淡出"
                    tip: "切歌或暂停时对音量淡入淡出，避免爆音"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.fadeEnabled
                        onToggled: {
                            Config.fadeEnabled = !Config.fadeEnabled
                        }
                    }
                }

                SettingItemCard {
                    label: "淡变时长"
                    tip: "淡入淡出动画持续的毫秒数"
                    QSlider {
                        from: 0
                        to: 500
                        stepSize: 50
                        leftText: true
                        valueText: value.toString() + " ms"
                        value: Config.fadeMs
                        onMoved: {
                            Config.fadeMs = value
                        }
                    }
                }

                SettingItemCard {
                    label: "睡眠定时默认"
                    tip: "睡眠定时的默认倒计时分钟数"
                    QSlider {
                        from: 1
                        to: 120
                        stepSize: 1
                        leftText: true
                        valueText: value.toString() + " 分"
                        value: Config.sleepMinutes
                        onMoved: Config.sleepMinutes = value
                    }
                }

                SettingItemCard {
                    label: "随机避免最近"
                    tip: "随机播放时避免重复最近播放的歌曲数量"
                    QSlider {
                        from: 0
                        to: 5
                        stepSize: 1
                        leftText: true
                        valueText: value.toString() + " 首"
                        value: Config.shuffleAvoid
                        onMoved: Config.shuffleAvoid = value
                    }
                }

                SettingItemCard {
                    label: "恢复上次播放列表"
                    tip: "下次启动时恢复退出前的播放列表"
                    QSwitch {
                        letRight: true
                        switchTrue: Config.autoRestoreQueue
                        onToggled: Config.autoRestoreQueue = !Config.autoRestoreQueue
                    }
                }

                SettingItemCard {
                    label: "播放历史上限"
                    tip: "最多保留的播放历史条数"
                    QSlider {
                        from: 20
                        to: 500
                        stepSize: 20
                        leftText: true
                        valueText: value.toString() + " 条"
                        value: Config.historyLimit
                        onMoved: Config.historyLimit = value
                    }
                }

                SettingItemCard {
                    label: "清空播放历史"
                    tip: "清空全部播放历史记录"
                    QButton {
                        shadowEnabled: false
                        fontSize: Style.textTip
                        radius: Style.labelRadius
                        text: "清空"
                        buttonColor: "#fa4642"
                        textColor: "#ffffff"
                        onClicked: Options.dialog.openSimpleDialog("清空", "将清空全部播放历史，是否继续？",
                            function() {
                                Playback.clearHistory()
                                Options.warned("已清空播放历史", 1)
                            })
                    }
                    bottomLine: false
                }
            }
        }
    }
}
