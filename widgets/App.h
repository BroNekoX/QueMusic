#pragma once

#include <QGuiApplication>
#include <QClipboard>
#include <QObject>
#include <QPointer>
#include <QRect>
#include <QWindow>
#include <QtQmlIntegration/qqmlintegration.h>

class QQmlComponent;
class QQmlEngine;
class QJSEngine;
class QQuickWindow;
class MediaSession;
class SystemTrayManager;

class App final : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(App)
    QML_SINGLETON
    Q_PROPERTY(bool mediaControlsAvailable READ mediaControlsAvailable NOTIFY mediaControlsAvailableChanged FINAL)
    Q_PROPERTY(bool mediaControlsEnabled READ mediaControlsEnabled WRITE setMediaControlsEnabled NOTIFY mediaControlsEnabledChanged FINAL)
    Q_PROPERTY(QString mediaControlsName READ mediaControlsName CONSTANT FINAL)

public:
    static App *create(QQmlEngine *engine, QJSEngine *);
    static App *instance();

    bool mediaControlsAvailable() const;
    bool mediaControlsEnabled() const;
    void setMediaControlsEnabled(bool on);
    QString mediaControlsName() const;

    Q_INVOKABLE void requestClose();
    Q_INVOKABLE void showMainWindow();
    Q_INVOKABLE void quit();

    // 字体只影响之后创建的 QML Text，换字体后重建主窗口让界面用上新字体
    Q_INVOKABLE void reloadUi();

signals:
    void mediaControlsAvailableChanged();
    void mediaControlsEnabledChanged();

private:
    explicit App(QQmlEngine *engine);
    ~App() override;

    void collapseToTray();
    void onGlobalFontChanged();
    void initTray();
    void finalizeQuit();
    bool trayAvailable() const;
    void showTrayMessage(const QString &title, const QString &body);
    void saveWindow();
    void trimMemory();
    void saveLastSong();
    void playStartupFile();

    QQmlEngine *m_engine;
    QQmlComponent *m_component;
    QPointer<QQuickWindow> m_window;
    SystemTrayManager *m_tray = nullptr;
    MediaSession *m_media = nullptr;
    QWindow::Visibility m_restoreVisibility = QWindow::Windowed;
    QRect m_normalGeometry;
    QRect m_pendingGeometry;
    bool m_collapsing = false;
    bool m_quitting = false;
    bool m_showPending = false;
    bool m_trayTipShown = false;

public slots:
    void copyText(const QString &text) {
        QGuiApplication::clipboard()->setText(text);
    }
    QString pasteText() {
        return QGuiApplication::clipboard()->text();
    }
};
