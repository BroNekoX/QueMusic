// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "DesktopLyricsWindow.h"

#include "AppSettings.h"
#include "MusicApiService.h"
#include "audio/AudioEngine.h"
#include "playback/Playback.h"

#include <QCloseEvent>
#include <QEnterEvent>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QHideEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QShowEvent>
#include <QTimer>
#include <QWindow>

namespace {

// 当前行主题色：QML 原版使用 Theme.themeColor（components/Theme.qml 默认 #3481fa）。
// Widgets 版不随 QML 主题联动，本期固定为默认主题色，后续可同步。
const QColor kAccentColor(0x34, 0x81, 0xfa);
// 悬浮工具条配色（对照 DesktopLyrics.qml :176-273）
const QColor kToolbarBg(0x00, 0x00, 0x00, 0x88);     // QML #88000000
const QColor kToolbarText(0xfa, 0xfa, 0xfa);         // QML #fffafafa
const QColor kToolbarIcon(0xfd, 0xfd, 0xfd);         // QML #fffdfdfd
const QColor kButtonHover(0xfa, 0xfa, 0xfa, 0x66);   // QML hoverColor #66fafafa
// 歌词阴影（QML MultiEffect shadowColor）
const QColor kCurrentShadow(0x00, 0x00, 0x00, 0x41); // 当前行 #41000000
const QColor kNextShadow(0x00, 0x00, 0x00, 0x56);    // 下一行 #56000000
const QColor kNextLineColor(0xfc, 0xfc, 0xfc);       // 下一行文字 #fcfcfc

// feather 图标族名：QML Fonts.qml 用的同一份字体资源
QString loadIconFontFamily()
{
    const int fontId = QFontDatabase::addApplicationFont(
        QStringLiteral(":/QueMusic/resources/fonts/feather.ttf"));
    const QString family =
        fontId >= 0 ? QFontDatabase::applicationFontFamilies(fontId).value(0)
                    : QString();
    return family.isEmpty() ? QStringLiteral("feather") : family;
}

} // namespace

DesktopLyricsWindow::DesktopLyricsWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("QueMusic 桌面歌词"));
    // QML flags: FramelessWindowHint | WindowStaysOnTopHint，另加 Tool 避免进任务栏
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(640, 160);          // QML width/height
    setMinimumSize(480, 120);  // QML minimumWidth/minimumHeight

    // 与 main.cpp 相同的 C++ 侧取单例方式（QML 单例工厂幂等返回既有实例）
    m_playback = Playback::create(nullptr, nullptr);
    m_player = m_playback ? m_playback->player() : nullptr;
    m_iconFamily = loadIconFontFamily();

    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(240);  // QML Timer interval: 240
    connect(m_tickTimer, &QTimer::timeout, this, &DesktopLyricsWindow::updateLyricIndex);

    // 歌词数据变化（换歌 / 在线加载完成）→ 重算下标（QML onLyricsDataChanged）
    if (auto *api = MusicApiService::instance()) {
        connect(api, &MusicApiService::lyricsDataChanged,
                this, &DesktopLyricsWindow::updateLyricIndex);
    }

    if (m_player) {
        // 播放位置/总时长变化 → 刷新工具条时间文本（QML 属性绑定按 20Hz 刷新）
        connect(m_player, &AudioEngine::positionChanged, this, [this] { update(); });
        connect(m_player, &AudioEngine::durationChanged, this, [this] { update(); });
        // onMedia 变化（有媒体/无媒体）→ 计时器启停（QML Timer.running 绑定）
        connect(m_player, &AudioEngine::mediaStatusChanged,
                this, &DesktopLyricsWindow::updateTickTimer);
    }

    // 工具条歌名-歌手与播放/暂停图标
    if (m_playback) {
        connect(m_playback, &Playback::musicTitleChanged, this, [this] { update(); });
        connect(m_playback, &Playback::musicArtistChanged, this, [this] { update(); });
        connect(m_playback, &Playback::inplayChanged, this, [this] { update(); });
    }

    // 显隐跟随设置开关（设置页开关 / 关闭按钮共同驱动）
    if (auto *settings = AppSettings::instance()) {
        connect(settings, &AppSettings::desktopLyricsOnChanged, this, [this] {
            auto *s = AppSettings::instance();
            setVisible(s && s->desktopLyricsOn());
        });
        if (settings->desktopLyricsOn())
            show();
    }

    updateLyricIndex();  // QML Component.onCompleted: updateCurrentIndex()
}

DesktopLyricsWindow::~DesktopLyricsWindow() = default;

// ---------------------------------------------------------------------------
// 歌词下标推进：移植 DesktopLyrics.qml:34-48 的增量算法
// （播放时下标只前进，seek 回退或换歌才从头扫）
// ---------------------------------------------------------------------------
void DesktopLyricsWindow::updateLyricIndex()
{
    auto *api = MusicApiService::instance();
    const QVariantList data = api ? api->lyricsData() : QVariantList();
    const qint64 pos = m_player ? m_player->position() : 0;

    if (data.isEmpty()) {
        m_currentIndex = -1;
        m_nextIndex = -1;
        update();
        return;
    }

    int idx = m_currentIndex;
    if (idx < 0 || idx >= data.size()
        || data.at(idx).toMap().value(QStringLiteral("time")).toLongLong() > pos)
        idx = 0;  // 换歌或往回跳转才从头扫
    while (idx + 1 < data.size()
           && pos >= data.at(idx + 1).toMap().value(QStringLiteral("time")).toLongLong())
        ++idx;
    m_currentIndex = idx;
    m_nextIndex = qMin(idx + 1, data.size() - 1);
    update();
}

// QML Timer.running: Playback.player.onMedia && visible
void DesktopLyricsWindow::updateTickTimer()
{
    const bool shouldRun = m_player && m_player->onMedia() && isVisible();
    if (shouldRun && !m_tickTimer->isActive())
        m_tickTimer->start();
    else if (!shouldRun && m_tickTimer->isActive())
        m_tickTimer->stop();
}

// 关闭按钮与 closeEvent 统一走这里：写回开关 false，由 NOTIFY 联动隐藏
void DesktopLyricsWindow::requestClose()
{
    auto *settings = AppSettings::instance();
    if (settings && settings->desktopLyricsOn())
        settings->setDesktopLyricsOn(false);
    else
        hide();  // 开关本来就是关（如 Alt+F4 强关）时兜底直接隐藏
}

void DesktopLyricsWindow::activateToolbarButton(ToolbarButton button)
{
    if (!m_playback)
        return;
    switch (button) {
    case ButtonPrevious:
        m_playback->previous();
        break;
    case ButtonPlayPause:
        m_playback->togglePlay();
        break;
    case ButtonNext:
        m_playback->next(false);  // QML Playback.next(false)
        break;
    case ButtonClose:
        requestClose();
        break;
    case ButtonNone:
        break;
    }
}

// ---------------------------------------------------------------------------
// 命中矩形与缩放热区
// ---------------------------------------------------------------------------
QRect DesktopLyricsWindow::toolbarButtonRect(ToolbarButton button) const
{
    // 中间三键：QML Row y=12 spacing=6，每键 36x36，水平居中
    const int rowWidth = 36 * 3 + 6 * 2;
    const int startX = (width() - rowWidth) / 2;
    switch (button) {
    case ButtonPrevious:
        return QRect(startX, 12, 36, 36);
    case ButtonPlayPause:
        return QRect(startX + 42, 12, 36, 36);
    case ButtonNext:
        return QRect(startX + 84, 12, 36, 36);
    case ButtonClose:
        return QRect(width() - 54, 12, 36, 36);  // QML closeButton x: width - 54
    case ButtonNone:
        break;
    }
    return QRect();
}

DesktopLyricsWindow::ToolbarButton DesktopLyricsWindow::toolbarButtonAt(const QPoint &pos) const
{
    const ToolbarButton buttons[] = {ButtonPrevious, ButtonPlayPause, ButtonNext, ButtonClose};
    for (ToolbarButton button : buttons) {
        if (toolbarButtonRect(button).contains(pos))
            return button;
    }
    return ButtonNone;
}

// 对照 QML :116-173 的五个缩放热区（右下角 12x12 优先级最高，先判定）
bool DesktopLyricsWindow::resizeEdgesAt(const QPoint &pos, Qt::Edges *edgesOut) const
{
    const int w = width();
    const int h = height();
    Qt::Edges edges;
    if (pos.x() >= w - 12 && pos.y() >= h - 12)
        edges = Qt::RightEdge | Qt::BottomEdge;              // 右下角斜向缩放
    else if (pos.x() >= w - 10 && pos.y() < h - 12)
        edges = Qt::RightEdge;                               // 右侧缩放
    else if (pos.x() < 10 && pos.y() < h - 10)
        edges = Qt::LeftEdge;                                // 左侧缩放
    else if (pos.y() >= h - 10 && pos.x() < w - 12)
        edges = Qt::BottomEdge;                              // 底边缩放
    else if (pos.y() < 10 && pos.x() < w - 10)
        edges = Qt::TopEdge;                                 // 顶边缩放
    if (edgesOut)
        *edgesOut = edges;
    return edges != Qt::Edges();
}

// ---------------------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------------------
void DesktopLyricsWindow::paintEvent(QPaintEvent *)
{
    if (!m_playback)
        return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    const QRect bounds = rect();
    // QML property lyricSize: height / 4 - 18
    const int lyricSize = qMax(1, bounds.height() / 4 - 18);

    // ---- 悬浮工具条：半透明黑圆角覆盖层（QML Rectangle #88000000 radius 12）----
    if (m_hovered) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(kToolbarBg);
        painter.drawRoundedRect(bounds, 12, 12);

        // 左侧：歌名 - 歌手（省略）
        QFont labelFont = font();
        labelFont.setPixelSize(12);  // Style.text
        const QRect titleRect(20, 12, bounds.width() / 2 - 80, 36);
        const QString titleText = m_playback->musicTitle()
            + QStringLiteral(" - ") + m_playback->musicArtist();
        painter.setFont(labelFont);
        painter.setPen(kToolbarText);
        painter.drawText(titleRect, Qt::AlignVCenter,
                         painter.fontMetrics().elidedText(titleText, Qt::ElideRight,
                                                          titleRect.width()));

        // 中间：上一首 / 播放暂停 / 下一首
        drawToolbarButton(&painter, toolbarButtonRect(ButtonPrevious),
                          QStringLiteral("\uF0DC"), 15, m_hoveredButton == ButtonPrevious);
        drawToolbarButton(&painter, toolbarButtonRect(ButtonPlayPause),
                          m_playback->inplay() ? QStringLiteral("\uF02F")
                                               : QStringLiteral("\uF00E"),
                          16, m_hoveredButton == ButtonPlayPause);
        drawToolbarButton(&painter, toolbarButtonRect(ButtonNext),
                          QStringLiteral("\uF0D9"), 15, m_hoveredButton == ButtonNext);

        // 右侧：已播 / 总长（Playback.fmt 格式化毫秒）
        QFont timeFont = font();
        timeFont.setPixelSize(13);
        const QString timeText = m_playback->fmt(m_player ? m_player->position() : 0)
            + QStringLiteral(" / ")
            + m_playback->fmt(m_player ? m_player->duration() : 0);
        painter.setFont(timeFont);
        painter.setPen(kToolbarIcon);
        const int timeWidth = painter.fontMetrics().horizontalAdvance(timeText);
        painter.drawText(QRect(bounds.width() - 68 - timeWidth, 12, timeWidth, 36),
                         Qt::AlignVCenter, timeText);

        // 关闭按钮
        drawToolbarButton(&painter, toolbarButtonRect(ButtonClose),
                          QStringLiteral("\uF025"), 15, m_hoveredButton == ButtonClose);
    }

    // ---- 双行歌词（QML Column y=60 spacing=8）----
    auto *api = MusicApiService::instance();
    const QVariantList data = api ? api->lyricsData() : QVariantList();
    const auto lineText = [data](int index) -> QString {
        if (index < 0 || index >= data.size())
            return QString();
        return data.at(index).toMap().value(QStringLiteral("text")).toString();
    };

    // 当前行：大号粗体 + 主题色；无有效下标时显示占位文案（QML :70）
    QString currentText = lineText(m_currentIndex);
    if (m_currentIndex < 0 || m_currentIndex >= data.size())
        currentText = QStringLiteral("🎵 纯音乐，请欣赏");
    QFont currentFont = font();
    currentFont.setPixelSize(qMax(1, int(lyricSize * 1.2)));
    currentFont.setBold(true);
    const QFontMetrics currentMetrics(currentFont);
    const QRect currentRect(0, 60, bounds.width(), currentMetrics.height());
    drawShadowedText(&painter, currentRect, currentFont, kAccentColor, currentText,
                     kCurrentShadow, 2);

    // 下一行：小一号、白色，仅在有内容时绘制（QML MultiEffect visible 绑定）
    const QString nextText = lineText(m_nextIndex);
    if (!nextText.isEmpty()) {
        QFont nextFont = font();
        nextFont.setPixelSize(qMax(1, lyricSize));
        nextFont.setBold(true);
        const QFontMetrics nextMetrics(nextFont);
        const QRect nextRect(0, 60 + currentMetrics.height() + 8, bounds.width(),
                             nextMetrics.height());
        drawShadowedText(&painter, nextRect, nextFont, kNextLineColor, nextText,
                         kNextShadow, 2);
    }
}

// QML MultiEffect 的柔和阴影用「多方向偏移重绘 + 正文覆盖」近似实现
void DesktopLyricsWindow::drawShadowedText(QPainter *painter, const QRect &rect,
                                           const QFont &font, const QColor &color,
                                           const QString &text, const QColor &shadowColor,
                                           int shadowOffset) const
{
    if (text.isEmpty() || rect.width() <= 0)
        return;
    const QString elided =
        QFontMetrics(font).elidedText(text, Qt::ElideRight, rect.width());
    static const QPoint kDirections[8] = {
        {1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, 1}, {1, -1}, {-1, -1}
    };
    painter->save();
    painter->setFont(font);
    painter->setPen(shadowColor);
    for (const QPoint &direction : kDirections) {
        painter->drawText(rect.translated(direction.x() * shadowOffset,
                                          direction.y() * shadowOffset),
                          Qt::AlignHCenter | Qt::AlignVCenter, elided);
    }
    painter->setPen(color);
    painter->drawText(rect, Qt::AlignHCenter | Qt::AlignVCenter, elided);
    painter->restore();
}

void DesktopLyricsWindow::drawToolbarButton(QPainter *painter, const QRect &rect,
                                            const QString &glyph, int iconSize,
                                            bool hovered) const
{
    if (rect.isEmpty())
        return;
    if (hovered) {  // QML hoverColor #66fafafa 圆角覆盖
        painter->setPen(Qt::NoPen);
        painter->setBrush(kButtonHover);
        painter->drawRoundedRect(rect, 12, 12);
    }
    QFont iconFont = font();
    iconFont.setFamily(m_iconFamily);
    iconFont.setPixelSize(iconSize);
    painter->setFont(iconFont);
    painter->setPen(kToolbarIcon);
    painter->drawText(rect, Qt::AlignCenter, glyph);
}

// ---------------------------------------------------------------------------
// 鼠标交互：缩放热区 > 工具条按钮 > 全窗口拖动
// ---------------------------------------------------------------------------
void DesktopLyricsWindow::mousePressEvent(QMouseEvent *event)
{
    const QPoint pos = event->position().toPoint();
    Qt::Edges edges;
    if (resizeEdgesAt(pos, &edges)) {
        // 五向缩放：QWidget 需经 windowHandle()（已 show 才非空）
        if (QWindow *window = windowHandle())
            window->startSystemResize(edges);
        return;
    }
    const ToolbarButton button = toolbarButtonAt(pos);
    if (button != ButtonNone) {
        m_pressedButton = button;
        update();
        return;
    }
    if (QWindow *window = windowHandle())
        window->startSystemMove();  // 全窗口拖动
}

void DesktopLyricsWindow::mouseReleaseEvent(QMouseEvent *event)
{
    const ToolbarButton button = m_pressedButton;
    m_pressedButton = ButtonNone;
    update();
    if (button != ButtonNone
        && toolbarButtonAt(event->position().toPoint()) == button)
        activateToolbarButton(button);  // 与 QML clicked 语义一致：抬起且仍在按钮内
}

void DesktopLyricsWindow::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint pos = event->position().toPoint();
    m_hovered = true;
    Qt::Edges edges;
    ToolbarButton hovered = ButtonNone;
    if (resizeEdgesAt(pos, &edges)) {
        Qt::CursorShape shape = Qt::ArrowCursor;
        if ((edges & (Qt::LeftEdge | Qt::RightEdge))
            && (edges & (Qt::TopEdge | Qt::BottomEdge)))
            shape = Qt::SizeFDiagCursor;
        else if (edges & (Qt::LeftEdge | Qt::RightEdge))
            shape = Qt::SizeHorCursor;
        else if (edges & (Qt::TopEdge | Qt::BottomEdge))
            shape = Qt::SizeVerCursor;
        setCursor(shape);
    } else {
        hovered = toolbarButtonAt(pos);
        setCursor(hovered != ButtonNone ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }
    if (hovered != m_hoveredButton) {
        m_hoveredButton = hovered;
        update();
    }
}

void DesktopLyricsWindow::enterEvent(QEnterEvent *)
{
    m_hovered = true;
    update();
}

void DesktopLyricsWindow::leaveEvent(QEvent *)
{
    m_hovered = false;
    m_hoveredButton = ButtonNone;
    m_pressedButton = ButtonNone;
    update();
}

void DesktopLyricsWindow::showEvent(QShowEvent *event)
{
    updateTickTimer();  // QML Timer.running = onMedia && visible
    QWidget::showEvent(event);
}

void DesktopLyricsWindow::hideEvent(QHideEvent *event)
{
    updateTickTimer();
    QWidget::hideEvent(event);
}

void DesktopLyricsWindow::closeEvent(QCloseEvent *event)
{
    // 不真正关闭：写回开关 false，与关闭按钮同路径（QML onClosing 语义）
    event->ignore();
    requestClose();
}
