#ifndef API_H
#define API_H

#include <QObject>
#include <QVariantMap>

class Api : public QObject
{
    Q_OBJECT
public:
    explicit Api(QObject *parent = nullptr);

    Q_INVOKABLE QVariantMap aaaaaaaaaaaaaaaaaaaa(QVariantMap);

    Q_INVOKABLE QVariantMap bbbbbbbbbbbbbbbbbbbb(QVariantMap);

signals:
};

#endif // API_H
