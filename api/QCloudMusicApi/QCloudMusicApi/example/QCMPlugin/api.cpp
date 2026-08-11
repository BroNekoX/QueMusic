#include "api.h"

#include <QDebug>

Api::Api(QObject *parent)
    : QObject{parent}
{}

QVariantMap Api::aaaaaaaaaaaaaaaaaaaa(QVariantMap query)
{
    return {
        { "status", 200 },
        { "body", QVariantMap
            { { "A method from plugin", query } }
        }
    };
}

QVariantMap Api::bbbbbbbbbbbbbbbbbbbb(QVariantMap query)
{
    qDebug() << "bbbbbbbbbbbbbbbbbbbb";
    return {
        { "status", 200 },
        { "body", QVariantMap
            { { "bbbbbbbbbbbbbbbbbbbb", query } }
        }
    };;
}
