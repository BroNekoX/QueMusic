// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
#include "DesktopMiniPlayerWindow.h"

#include "AppSettings.h"
#include "audio/AudioEngine.h"
#include "playback/Playback.h"

#include <QCloseEvent>
#include <QEnterEvent>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QUrl>
#include <QWindow>

namespace {

// 配色对照 components/Theme.qml 的 QML 引用值（Widgets 版不随主题联动，本期固定）
const QColor kCardColor(0xfd, 0xfd, 0xff);            // playerCard：Theme.primaryColor #fdfdff
const QColor kCardBorder(0xe9, 0xe9, 0xee);           // 边框：Theme.sideColor #e9e9ee
const QColor kTitleColor(0x1d, 0x1d, 0x1f);           // 标题：Theme.fontColor #1d1d1f
const QColor kTextColor(0x6e, 0x6e, 0x73);            // 歌手/时间/图标：Theme.textColor #6e6e73
const QColor kGrooveColor(0xf3, 0xf5, 0xf8);          // 进度槽：Theme.secondaryColor #f3f5f8
const QColor kThemeColor(0x34, 0x81, 0xfa);           // 进度填充/手柄：Theme.themeColor #3481fa
const QColor kPlayButtonBg(0xf3, 0xf5, 0xf8, 0xc4);   // 播放键底色：Theme.secondaryBlurColor #c4f3f5f8
const QColor kHoverColor(0x00, 0x00, 0x00, 0x0a);     // 悬停：Theme.hoverColor #0a000000
const QColor kCloseHover(0xff, 0x66, 0x66, 0x66);     // 关闭键悬停：QML Qt.rgba(1.0, 0.4, 0.4, 0.4)

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

DesktopMiniPlayerWindow::DesktopMiniPlayerWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle(QStringLiteral("QueMusic 桌面播放器"));
    // QML flags: FramelessWindowHint | WindowStaysOnTopHint，另加 Tool 避免进任务栏
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    resize(340, 148);          // QML width/height
    setMinimumSize(320, 72);   // QML minimumWidth/minimumHeight
    setMaximumSize(480, 160);  // QML maximumWidth/maximumHeight

    // 与 main.cpp 相同的 C++ 侧取单例方式（QML 单例工厂幂等返回既有实例）
    m_playback = Playback::create(nullptr, nullptr);
    m_player = m_playback ? m_playback->player() : nullptr;
    m_iconFamily = loadIconFontFamily();

    // 歌名/歌手/播放状态/循环模式 → 重绘
    if (m_playback) {
        connect(m_playback, &Playback::musicTitleChanged, this, [this] { update(); });
        connect(m_playback, &Playback::musicArtistChanged, this, [this] { update(); });
        connect(m_playback, &Playback::inplayChanged, this, [this] { update(); });
        connect(m_playback, &Playback::cycleIndexChanged, this, [this] { update(); });
    }

    if (m_player) {
        // 播放位置（约 20Hz）与总时长 → 进度条 / 时间标签
        connect(m_player, &AudioEngine::positionChanged, this, [this] { update(); });
        connect(m_player, &AudioEngine::durationChanged, this, [this] { update(); });
        // 封面跟随 player.urlStr（QML QPicture image 绑定）
        connect(m_player, &AudioEngine::urlStrChanged,
                this, &DesktopMiniPlayerWindow::refreshCoverImage);
    }
    refreshCoverImage();

    // 注意：不再连接 AppSettings::desktopMiniPlayerOnChanged 自管显隐——创建/
    // 显示/销毁统一由 cpp/resident/DesktopWidgetsController 响应开关完成
    // （按开关创建、关闭即销毁），构造出的实例由控制器紧随其后 show()
}

DesktopMiniPlayerWindow::~DesktopMiniPlayerWindow() = default;

// ---------------------------------------------------------------------------
// 布局几何（对照 DesktopPlayerWindow.qml :88-299 的坐标定义）
// ---------------------------------------------------------------------------
bool DesktopMiniPlayerWindow::isSmallMode() const
{
    return height() < 132;  // QML readonly property bool isSmall: height < 132
}

QRect DesktopMiniPlayerWindow::coverRect() const
{
    // QML QPicture：x=16；小窗 52x52 垂直居中，大窗 64x64 y=16
    if (isSmallMode())
        return QRect(16, (height() - 52) / 2, 52, 52);
    return QRect(16, 16, 64, 64);
}

QRect DesktopMiniPlayerWindow::sliderRect() const
{
    // 大窗：x=96 y=封面y+35 宽=width-112（QML :127-130）
    if (!isSmallMode())
        return QRect(96, coverRect().y() + 35, width() - 112, 16);
    // 小窗：与两个时间标签同行，位于「已播时间」和「总时长」之间
    const QFontMetrics metrics(timeLabelFont());
    const int left = 84 + metrics.horizontalAdvance(currentTimeText()) + 6;
    const int right = width() - 16 - metrics.horizontalAdvance(totalTimeText()) - 6;
    return QRect(left, coverRect().y() + 35, qMax(0, right - left), 16);
}

QRect DesktopMiniPlayerWindow::controlButtonRect(ControlButton button) const
{
    // QML dpControlRow：每键方形（边长=行高），大窗 36 高 36、垂直 y=height-52 水平居中；
    // 小窗 28 高 28、y=封面y+2、右对齐留 44 边距（QML :192-196）
    const int size = isSmallMode() ? 28 : 36;
    const int rowWidth = size * 5;
    const int x = isSmallMode() ? width() - rowWidth - 44 : (width() - rowWidth) / 2;
    const int y = isSmallMode() ? coverRect().y() + 2 : height() - 52;
    int offset = 0;
    switch (button) {
    case ButtonCycle:
        offset = 0;
        break;
    case ButtonPrevious:
        offset = size;
        break;
    case ButtonPlayPause:
        offset = size * 2;
        break;
    case ButtonNext:
        offset = size * 3;
        break;
    case ButtonPinTop:
        offset = size * 4;
        break;
    case ButtonClose: {
        // 右上角关闭键（QML closeButton :279-299）：大窗 32x32 y=12，小窗 28x28 与控制行同排
        const bool small = isSmallMode();
        const int closeSize = small ? 28 : 32;
        return QRect(width() - 44, small ? coverRect().y() + 2 : 12, closeSize, closeSize);
    }
    case ButtonNone:
        return QRect();
    }
    return QRect(x + offset, y, size, size);
}

DesktopMiniPlayerWindow::ControlButton
DesktopMiniPlayerWindow::controlButtonAt(const QPoint &pos) const
{
    const ControlButton buttons[] = {ButtonCycle, ButtonPrevious, ButtonPlayPause,
                                     ButtonNext, ButtonPinTop, ButtonClose};
    for (ControlButton button : buttons) {
        if (controlButtonRect(button).contains(pos))
            return button;
    }
    return ButtonNone;
}

// 对照 QML :31-66 的三向缩放热区（右下角 10x10 优先级最高，先判定）
bool DesktopMiniPlayerWindow::resizeEdgesAt(const QPoint &pos, Qt::Edges *edgesOut) const
{
    const int w = width();
    const int h = height();
    Qt::Edges edges;
    if (pos.x() >= w - 10 && pos.y() >= h - 10)
        edges = Qt::RightEdge | Qt::BottomEdge;  // 右下角斜向缩放
    else if (pos.x() >= w - 10 && pos.y() < h - 10)
        edges = Qt::RightEdge;                   // 右侧缩放
    else if (pos.y() >= h - 10 && pos.x() < w - 10)
        edges = Qt::BottomEdge;                  // 底边缩放
    if (edgesOut)
        *edgesOut = edges;
    return edges != Qt::Edges();
}

// ---------------------------------------------------------------------------
// 关闭 / 按钮 / 进度条行为
// ---------------------------------------------------------------------------
// 关闭按钮与 closeEvent 统一走这里：写回开关 false，DesktopWidgetsController
// 响应 NOTIFY 完成销毁（关闭即销毁，本类不做任何自管显隐）
void DesktopMiniPlayerWindow::requestClose()
{
    auto *settings = AppSettings::instance();
    if (settings && settings->desktopMiniPlayerOn())
        settings->setDesktopMiniPlayerOn(false);  // NOTIFY → 控制器销毁窗口
    else
        hide();  // 开关本来就是关（如 Alt+F4 强关）时兜底直接隐藏
}

void DesktopMiniPlayerWindow::activateButton(ControlButton button)
{
    if (!m_playback)
        return;
    switch (button) {
    case ButtonCycle:  // QML: cycleIndex < 3 ? cycleIndex + 1 : 0
        m_playback->setCycleIndex(m_playback->cycleIndex() < 3 ? m_playback->cycleIndex() + 1 : 0);
        break;
    case ButtonPrevious:
        m_playback->previous();
        break;
    case ButtonPlayPause:
        m_playback->togglePlay();
        break;
    case ButtonNext:
        m_playback->next(false);  // QML Playback.next(false)
        break;
    case ButtonPinTop:
        // QML :266-274：运行时切换置顶；Widgets 下 setWindowFlag 会隐藏窗口，需重新 show
        m_topWindow = !m_topWindow;
        setWindowFlag(Qt::WindowStaysOnTopHint, m_topWindow);
        show();
        break;
    case ButtonClose:
        requestClose();
        break;
    case ButtonNone:
        break;
    }
}

qreal DesktopMiniPlayerWindow::sliderRatio() const
{
    // QML Slider to: duration > 0 ? duration : 1
    const qint64 to = m_player && m_player->duration() > 0 ? m_player->duration() : 1;
    const qint64 value = m_seekDragging ? m_seekValue : (m_player ? m_player->position() : 0);
    return qBound(0.0, qreal(value) / qreal(to), 1.0);
}

void DesktopMiniPlayerWindow::seekToPixel(int x)
{
    const QRect rect = sliderRect();
    if (rect.width() <= 0 || !m_player)
        return;
    const qint64 to = m_player->duration() > 0 ? m_player->duration() : 1;
    const qreal ratio = qBound(0.0, qreal(x - rect.x()) / qreal(rect.width()), 1.0);
    const qint64 value = qint64(ratio * qreal(to));
    if (value != m_seekValue) {
        m_seekValue = value;
        m_player->setPosition(value);  // 与 QML onMoved 一致：值变化即写回位置
        update();
    }
}

void DesktopMiniPlayerWindow::refreshCoverImage()
{
    m_coverImage = QImage();
    const QString source = m_player ? m_player->urlStr() : QString();
    if (!source.isEmpty()) {
        // 与 QML Image 能力对齐的三类来源：qrc:/、file:///、裸本地路径；
        // 远程 http(s) 封面本期不做异步拉取，直接落到默认占位图
        const QUrl url(source);
        QString path;
        if (url.scheme() == QLatin1String("qrc"))
            path = QLatin1Char(':') + url.path();
        else if (url.scheme() == QLatin1String("file"))
            path = url.toLocalFile();
        else if (url.scheme().isEmpty())
            path = source;
        if (!path.isEmpty())
            m_coverImage.load(path);
    }
    if (m_coverImage.isNull())
        m_coverImage.load(QStringLiteral(":/QueMusic/resources/app/musicpic.png"));  // QML fallbackSource
    update();
}

QString DesktopMiniPlayerWindow::currentTimeText() const
{
    const qint64 position = m_seekDragging ? m_seekValue : (m_player ? m_player->position() : 0);
    return m_playback ? m_playback->fmt(position) : QStringLiteral("0:00");
}

QString DesktopMiniPlayerWindow::totalTimeText() const
{
    return m_playback ? m_playback->fmt(m_player ? m_player->duration() : 0)
                      : QStringLiteral("0:00");
}

QFont DesktopMiniPlayerWindow::timeLabelFont() const
{
    QFont labelFont = font();
    labelFont.setPixelSize(11);  // QML font.pixelSize: 11
    return labelFont;
}

QFont DesktopMiniPlayerWindow::iconFont(const QString &family, int pixelSize)
{
    QFont iconFont;
    iconFont.setFamily(family);
    iconFont.setPixelSize(pixelSize);
    return iconFont;
}

// ---------------------------------------------------------------------------
// 绘制
// ---------------------------------------------------------------------------
void DesktopMiniPlayerWindow::paintEvent(QPaintEvent *)
{
    if (!m_playback)
        return;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // 播放器卡片（QML playerCard：radius 12、primaryColor 底、1px sideColor 边）
    painter.setPen(QPen(kCardBorder, 1.0));
    painter.setBrush(kCardColor);
    painter.drawRoundedRect(QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5), 12, 12);

    drawCoverImage(&painter);

    // ---- 标题 / 歌手（QML :99-124）----
    const bool small = isSmallMode();
    const QRect cover = coverRect();
    const int textX = small ? 84 : 96;
    const int textWidth = small
        ? controlButtonRect(ButtonPinTop).right() - 8 - textX
        : width() - 112;

    QFont titleFont = font();
    titleFont.setPixelSize(14);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.setPen(kTitleColor);
    painter.drawText(QRect(textX, cover.y(), textWidth, 18), Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(m_playback->musicTitle(),
                                                      Qt::ElideRight, textWidth));

    QFont artistFont = font();
    artistFont.setPixelSize(12);
    painter.setFont(artistFont);
    painter.setPen(kTextColor);
    painter.drawText(QRect(textX, cover.y() + 18, textWidth, 16), Qt::AlignVCenter,
                     painter.fontMetrics().elidedText(m_playback->musicArtist(),
                                                      Qt::ElideRight, textWidth));

    // ---- 进度条（QML Slider :126-169）----
    const QRect slider = sliderRect();
    if (slider.width() > 0) {
        const qreal ratio = sliderRatio();
        const QRectF groove(slider.x(), slider.y() + (slider.height() - 4) / 2.0,
                            slider.width(), 4);
        painter.setPen(Qt::NoPen);
        painter.setBrush(kGrooveColor);
        painter.drawRoundedRect(groove, 2, 2);
        painter.setBrush(kThemeColor);
        painter.drawRoundedRect(QRectF(groove.x(), groove.y(), groove.width() * ratio, 4), 2, 2);
        // 手柄：12x12 圆，主题色填充 + 3px primaryColor 描边
        const QRectF handle(slider.x() + ratio * (slider.width() - 12),
                            slider.y() + (slider.height() - 12) / 2.0, 12, 12);
        painter.setBrush(kThemeColor);
        painter.setPen(QPen(kCardColor, 3.0));
        painter.drawEllipse(handle);
    }

    // ---- 时间标签（QML timeText / timeFullText :171-190）----
    const int timeY = small ? cover.y() + 35 : 70;
    painter.setFont(timeLabelFont());
    painter.setPen(kTextColor);
    const QFontMetrics timeMetrics = painter.fontMetrics();
    painter.drawText(QRect(small ? 84 : 96, timeY,
                           timeMetrics.horizontalAdvance(currentTimeText()) + 4, 16),
                     Qt::AlignVCenter, currentTimeText());
    const int fullX = width() - 16 - timeMetrics.horizontalAdvance(totalTimeText());
    painter.drawText(QRect(fullX, timeY,
                           timeMetrics.horizontalAdvance(totalTimeText()) + 4, 16),
                     Qt::AlignVCenter, totalTimeText());

    // ---- 控制按钮行（QML dpControlRow :192-277）----
    const int controlSize = small ? 28 : 36;
    const ControlButton order[] = {ButtonCycle, ButtonPrevious, ButtonPlayPause,
                                   ButtonNext, ButtonPinTop};
    for (ControlButton button : order) {
        const QRect buttonRect = controlButtonRect(button);
        // 背景：播放键常显 secondaryBlurColor 圆底，其余透明；悬停叠一层 hoverColor
        if (button == ButtonPlayPause) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(kPlayButtonBg);
            painter.drawRoundedRect(buttonRect, controlSize / 2, controlSize / 2);
        }
        if (m_hoveredButton == button) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(kHoverColor);
            painter.drawRoundedRect(buttonRect, controlSize / 2, controlSize / 2);
        }
        QString glyph;
        int iconSize = 16;
        switch (button) {
        case ButtonCycle:
            glyph = m_playback->cycleIcon();
            iconSize = 14;  // QML iconSize: Style.texticon - 2
            break;
        case ButtonPrevious:
            glyph = QStringLiteral("\uF0DC");
            iconSize = 16;  // QML iconSize: Style.texticon
            break;
        case ButtonPlayPause:
            glyph = m_playback->inplay() ? QStringLiteral("\uF02F") : QStringLiteral("\uF00E");
            iconSize = 18;  // QML iconSize: Style.texticon + 2
            break;
        case ButtonNext:
            glyph = QStringLiteral("\uF0D9");
            iconSize = 16;
            break;
        case ButtonPinTop:
            glyph = m_topWindow ? QStringLiteral("\uF003") : QStringLiteral("\uF05C");
            iconSize = 14;
            break;
        case ButtonNone:
        case ButtonClose:
            break;
        }
        painter.setFont(iconFont(m_iconFamily, iconSize));
        painter.setPen(kTextColor);
        painter.drawText(buttonRect, Qt::AlignCenter, glyph);
    }

    // ---- 关闭按钮（QML closeButton :279-299；命中区与绘制共用 controlButtonRect）----
    const QRect closeRect = controlButtonRect(ButtonClose);
    const int closeSize = closeRect.width();
    if (m_hoveredButton == ButtonClose) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(kCloseHover);
        painter.drawRoundedRect(closeRect, closeSize / 2, closeSize / 2);
    }
    painter.setFont(iconFont(m_iconFamily, 16));
    painter.setPen(kTextColor);
    painter.drawText(closeRect, Qt::AlignCenter, QStringLiteral("\uF025"));
}

void DesktopMiniPlayerWindow::drawCoverImage(QPainter *painter)
{
    const QRect rect = coverRect();
    painter->save();
    QPainterPath clip;
    clip.addRoundedRect(rect, 8, 8);  // QML QPicture radius: 8
    painter->setClipPath(clip);
    if (!m_coverImage.isNull()) {
        // QML Image fillMode: PreserveAspectCrop —— 等比放大铺满、居中裁剪
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        const QImage scaled = m_coverImage.scaled(rect.size(), Qt::KeepAspectRatioByExpanding,
                                                  Qt::SmoothTransformation);
        painter->drawImage(QPoint(rect.x() + (rect.width() - scaled.width()) / 2,
                                  rect.y() + (rect.height() - scaled.height()) / 2),
                           scaled);
    } else {
        painter->setPen(Qt::NoPen);
        painter->setBrush(kGrooveColor);
        painter->drawRoundedRect(rect, 8, 8);
    }
    painter->restore();
}

// ---------------------------------------------------------------------------
// 鼠标交互：缩放热区 > 控制按钮 > 进度条 > 全卡片拖动
// ---------------------------------------------------------------------------
void DesktopMiniPlayerWindow::mousePressEvent(QMouseEvent *event)
{
    const QPoint pos = event->position().toPoint();
    Qt::Edges edges;
    if (resizeEdgesAt(pos, &edges)) {
        // 三向缩放：QWidget 需经 windowHandle()（已 show 才非空）
        if (QWindow *window = windowHandle())
            window->startSystemResize(edges);
        return;
    }
    const ControlButton button = controlButtonAt(pos);
    if (button != ButtonNone) {
        m_pressedButton = button;
        update();
        return;
    }
    if (sliderRect().contains(pos)) {
        m_seekDragging = true;
        seekToPixel(pos.x());
        return;
    }
    if (QWindow *window = windowHandle())
        window->startSystemMove();  // 全卡片拖动（QML MouseArea anchors.fill margins:10）
}

void DesktopMiniPlayerWindow::mouseReleaseEvent(QMouseEvent *event)
{
    const ControlButton button = m_pressedButton;
    m_pressedButton = ButtonNone;
    if (m_seekDragging) {
        m_seekDragging = false;
        if (m_player)
            m_player->setPosition(m_seekValue);  // 拖动结束写回最终位置
        update();
        return;
    }
    update();
    if (button != ButtonNone
        && controlButtonAt(event->position().toPoint()) == button)
        activateButton(button);  // 与 QML clicked 语义一致：抬起且仍在按钮内
}

void DesktopMiniPlayerWindow::mouseMoveEvent(QMouseEvent *event)
{
    const QPoint pos = event->position().toPoint();
    Qt::Edges edges;
    ControlButton hovered = ButtonNone;
    if (resizeEdgesAt(pos, &edges)) {
        Qt::CursorShape shape = Qt::SizeVerCursor;
        if ((edges & (Qt::LeftEdge | Qt::RightEdge))
            && (edges & (Qt::TopEdge | Qt::BottomEdge)))
            shape = Qt::SizeFDiagCursor;
        else if (edges & (Qt::LeftEdge | Qt::RightEdge))
            shape = Qt::SizeHorCursor;
        setCursor(shape);
    } else if (m_seekDragging) {
        seekToPixel(pos.x());
        return;
    } else {
        hovered = controlButtonAt(pos);
        setCursor(hovered != ButtonNone || sliderRect().contains(pos)
                      ? Qt::PointingHandCursor
                      : Qt::ArrowCursor);
    }
    if (hovered != m_hoveredButton) {
        m_hoveredButton = hovered;
        update();
    }
}

void DesktopMiniPlayerWindow::enterEvent(QEnterEvent *)
{
    // 悬停高亮由 mouseMoveEvent 按 m_hoveredButton 逐按钮处理
}

void DesktopMiniPlayerWindow::leaveEvent(QEvent *)
{
    m_hoveredButton = ButtonNone;
    m_pressedButton = ButtonNone;
    update();
}

void DesktopMiniPlayerWindow::closeEvent(QCloseEvent *event)
{
    // 不真正关闭：写回开关 false，与关闭按钮同路径（QML onClosing 语义）
    event->ignore();
    requestClose();
}
