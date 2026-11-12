#include "App.h"

#include "AppSettings.h"
#include "Config.h"
#include "SmtcBridge.h"
#include "Style.h"
#include "SystemTrayManager.h"

#include "audio/AudioEngine.h"
#include "playback/Playback.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QEvent>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>

namespace {
App *s_instance = nullptr;

const QStringList &audioExtensions()
{
    static const QStringList extensions = {
        QStringLiteral("mp3"), QStringLiteral("flac"), QStringLiteral("wav"),
        QStringLiteral("m4a"), QStringLiteral("aac"), QStringLiteral("ogg"),
        QStringLiteral("oga"), QStringLiteral("opus"), QStringLiteral("ape"),
        QStringLiteral("wma"), QStringLiteral("aiff"), QStringLiteral("aif"),
        QStringLiteral("mka")
    };
    return extensions;
}
}

App *App::create(QQmlEngine *engine, QJSEngine *)
{
    if (!s_instance) {
        s_instance = new App(engine);
        QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
        s_instance->initTray();
    }
    return s_instance;
}

App *App::instance()
{
    return s_instance;
}

App::App(QQmlEngine *engine)
    : QObject(engine)
    , m_engine(engine)
    , m_component(new QQmlComponent(engine, QUrl(QStringLiteral("qrc:/QueMusic/main.qml")), this))
{
    if (m_component->isError())
        qWarning() << "[App] main.qml:" << m_component->errors();

    m_smtc = new SmtcBridge(this);
    connect(m_smtc, &SmtcBridge::availableChanged, this, &App::smtcAvailableChanged);

    QTimer::singleShot(400, this, &App::playStartupFile);

    // 换字体：排队刷新，避免在 QML 赋值调用栈里改界面对象
    auto *style = Style::instance();
    const auto scheduleFontRefresh = [this] {
        QTimer::singleShot(0, this, &App::onGlobalFontChanged);
    };
    connect(style, &Style::globalFontChanged, this, scheduleFontRefresh);
    connect(style, &Style::fontStyleChanged, this, scheduleFontRefresh);
}

App::~App()
{
    if (m_smtc)
        m_smtc->shutdown();
}

bool App::smtcAvailable() const
{
    return m_smtc && m_smtc->available();
}

void App::requestClose()
{
    if (Config::instance()->closeToManage() && trayAvailable()) {
        QTimer::singleShot(0, this, &App::collapseToTray);
        return;
    }
    quit();
}

void App::showMainWindow()
{
    if (m_collapsing) {
        m_showPending = true;
        return;
    }
    if (m_window) {
        if (!m_window->isVisible() || m_window->visibility() == QWindow::Minimized)
            m_window->showNormal();
        m_window->raise();
        m_window->requestActivate();
        return;
    }

    QObject *object = m_component->create();
    m_window = qobject_cast<QQuickWindow *>(object);
    if (!m_window) {
        delete object;
        qWarning() << "[App] main.qml create failed";
        return;
    }
    QQmlEngine::setObjectOwnership(m_window, QQmlEngine::CppOwnership);

    if (!m_pendingGeometry.isEmpty()) {
        m_window->setGeometry(m_pendingGeometry);
        m_pendingGeometry = QRect();
    } else if (Config::instance()->rememberWindow() && Config::instance()->winW() > 0) {
        m_window->setGeometry(Config::instance()->winX(), Config::instance()->winY(),
                              Config::instance()->winW(), Config::instance()->winH());
    }
    m_normalGeometry = m_window->geometry();

    const auto trackGeometry = [this] {
        if (m_window && m_window->visibility() == QWindow::Windowed)
            m_normalGeometry = m_window->geometry();
    };
    connect(m_window, &QWindow::xChanged, this, trackGeometry);
    connect(m_window, &QWindow::yChanged, this, trackGeometry);
    connect(m_window, &QWindow::widthChanged, this, trackGeometry);
    connect(m_window, &QWindow::heightChanged, this, trackGeometry);

    if (m_restoreVisibility == QWindow::Maximized)
        m_window->showMaximized();
    else if (m_restoreVisibility == QWindow::FullScreen)
        m_window->showFullScreen();
    else
        m_window->showNormal();

    m_window->raise();
    m_window->requestActivate();
}

void App::onGlobalFontChanged()
{
    Style::instance()->applyGlobalFont();
    QTimer::singleShot(0, this, &App::reloadUi);
}

void App::reloadUi()
{
    if (!m_window || m_collapsing)
        return;

    saveWindow();
    m_pendingGeometry = m_window->geometry();
    m_restoreVisibility = m_window->visibility();

    QQuickWindow *window = m_window;
    m_window = nullptr;
    window->hide();
    window->releaseResources();
    // 等旧窗口真正销毁后再建新的，避免两棵 QML 树短暂并存
    connect(window, &QObject::destroyed, this, [this] {
        if (!m_quitting)
            QTimer::singleShot(0, this, &App::showMainWindow);
    });
    window->deleteLater();
}

void App::collapseToTray()
{
    if (!m_window || m_collapsing)
        return;

    saveWindow();
    m_restoreVisibility = m_window->visibility();

    QQuickWindow *window = m_window;
    m_window = nullptr;
    m_collapsing = true;
    window->hide();
    window->releaseResources();
    connect(window, &QObject::destroyed, this, [this] {
        m_collapsing = false;
        if (m_showPending) {
            m_showPending = false;
            QTimer::singleShot(0, this, &App::showMainWindow);
        }
        QTimer::singleShot(0, this, &App::trimMemory);
        QTimer::singleShot(5000, this, [this] {
            if (!m_window && !m_collapsing)
                trimMemory();
        });
    });
    window->deleteLater();

    if (!m_trayTipShown && trayAvailable()) {
        m_trayTipShown = true;
        showTrayMessage(QStringLiteral("QueMusic"),
                        QStringLiteral("已最小化到系统托盘，点击托盘图标可恢复"));
    }
}

void App::quit()
{
    if (m_quitting)
        return;
    m_quitting = true;
    QMetaObject::invokeMethod(this, [this] { finalizeQuit(); }, Qt::QueuedConnection);
}

void App::finalizeQuit()
{
    saveWindow();
    saveLastSong();

    if (m_window) {
        QQuickWindow *window = m_window;
        m_window = nullptr;
        window->hide();
        window->releaseResources();
        window->deleteLater();
    }

    if (Playback *playback = Playback::create(nullptr, nullptr)) {
        if (AudioEngine *engine = playback->player())
            engine->stop();
        playback->saveQueue();
        playback->flush();
    }

    if (m_smtc)
        m_smtc->shutdown();
    if (m_tray)
        m_tray->hide();

    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QCoreApplication::quit();
}

void App::initTray()
{
    m_tray = new SystemTrayManager(this);
    connect(m_tray, &SystemTrayManager::showWindowRequested, this, &App::showMainWindow);
    connect(m_tray, &SystemTrayManager::quitRequested, this, &App::quit);

    Playback *playback = Playback::create(nullptr, nullptr);
    if (!playback)
        return;

    connect(m_tray, &SystemTrayManager::playPauseRequested, playback, &Playback::togglePlay);
    connect(m_tray, &SystemTrayManager::previousRequested, playback, &Playback::previous);
    connect(m_tray, &SystemTrayManager::nextRequested, playback,
            [playback] { playback->next(false); });

    const auto updateTray = [this, playback] {
        const bool hasTrack = !playback->musicHash().isEmpty();
        m_tray->setNowPlaying(hasTrack ? playback->musicTitle() : QString(),
                              hasTrack ? playback->musicArtist() : QString());
        m_tray->setPlaying(playback->inplay());
    };
    connect(playback, &Playback::musicTitleChanged, this, updateTray);
    connect(playback, &Playback::musicArtistChanged, this, updateTray);
    connect(playback, &Playback::musicHashChanged, this, updateTray);
    connect(playback, &Playback::inplayChanged, this, updateTray);
    updateTray();
}

bool App::trayAvailable() const
{
    return m_tray && m_tray->available();
}

void App::showTrayMessage(const QString &title, const QString &body)
{
    if (m_tray)
        m_tray->showMessage(title, body);
}

void App::saveWindow()
{
    Config *config = Config::instance();
    if (!m_window || !config->rememberWindow() || m_normalGeometry.width() <= 0)
        return;

    config->set_winX(m_normalGeometry.x());
    config->set_winY(m_normalGeometry.y());
    config->set_winW(m_normalGeometry.width());
    config->set_winH(m_normalGeometry.height());
}

void App::trimMemory()
{
    m_engine->collectGarbage();
    m_engine->trimComponentCache();
    m_engine->collectGarbage();
}

void App::saveLastSong()
{
    Playback *playback = Playback::create(nullptr, nullptr);
    if (!playback || playback->musicHash().isEmpty())
        return;

    Config *config = Config::instance();
    config->set_lastSongName(playback->musicTitle());
    config->set_lastSongArtist(playback->musicArtist());
    config->set_lastSongCover(playback->player()->urlStr());
    config->set_lastSongHash(playback->musicHash());
    config->set_lastSongSource(playback->musicSource());
}

void App::playStartupFile()
{
    const QStringList args = QCoreApplication::arguments();
    QString file;
    for (int i = 1; i < args.size(); ++i) {
        const QString arg = args.at(i).trimmed();
        if (arg.isEmpty() || arg.startsWith(QLatin1Char('-')))
            continue;
        const QString suffix = QFileInfo(arg).suffix().toLower();
        if (audioExtensions().contains(suffix)) {
            file = arg;
            break;
        }
    }
    if (file.isEmpty())
        return;

    const QString name = QUrl::fromPercentEncoding(QFileInfo(file).fileName().toUtf8());
    Playback *playback = Playback::create(nullptr, nullptr);
    if (playback)
        playback->playLocalSong(file, name);
}
