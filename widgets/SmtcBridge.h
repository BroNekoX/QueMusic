#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QWindow>

class WindowsSmtcManager;

class SmtcBridge final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged FINAL)

public:
    explicit SmtcBridge(QObject *parent = nullptr);
    ~SmtcBridge() override;

    bool available() const;
    void shutdown();

signals:
    void availableChanged();

private:
    void push();
    void pushInfo();
    void pushTimeline();
    void updateControls();

    WindowsSmtcManager *m_manager = nullptr;
    QWindow m_anchor;
    QElapsedTimer m_clock;
    qint64 m_lastTimeline = 0;
    qint64 m_lastPosition = 0;
    bool m_queued = false;
    bool m_infoQueued = false;
};
