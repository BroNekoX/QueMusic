// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors

#include "SelfCheck.h"

#include "ApiActions.h"
#include "ApiCommon.h"
#include "ApiResultRouter.h"
#include "LyricTimeline.h"
#include "MusicSource.h"
#include "OnlineListModel.h"
#include "QualityHashCache.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QModelIndex>
#include <QSet>

namespace {

class Report
{
public:
    void check(bool ok, const QString &name, const QString &detail = QString())
    {
        if (ok) {
            ++passed;
            return;
        }
        ++failed;
        issues.append(detail.isEmpty() ? name : name + QStringLiteral(": ") + detail);
    }

    int passed = 0;
    int failed = 0;
    QStringList issues;
};

class ProbeSource : public MusicSource
{
public:
    ProbeSource() : MusicSource(9999) {}
    QString sourceName() const override { return QStringLiteral("probe"); }

protected:
    void onSearchSongs(const QString &keyword, int type, int page, int pageSize) override
    {
        deliver(ApiAction::SearchSongs,
                QVariantMap{{QStringLiteral("keyword"), keyword},
                            {QStringLiteral("type"), type},
                            {QStringLiteral("page"), page},
                            {QStringLiteral("pageSize"), pageSize}},
                9999);
    }
};

void checkSourceList(const SelfCheckContext &context, Report &report)
{
    report.check(!context.sources.isEmpty(), QStringLiteral("source list is empty"));
    report.check(context.sourceIds.size() == context.sources.size(),
                 QStringLiteral("sourceIds size mismatch"));
    report.check(context.sourceNames.size() == context.sources.size(),
                 QStringLiteral("sourceNames size mismatch"));

    QSet<int> ids;
    QSet<int> declaredIds;
    for (const QVariant &value : context.sourceIds)
        declaredIds.insert(value.toInt());
    report.check(declaredIds.size() == context.sourceIds.size(),
                 QStringLiteral("duplicate sourceIds"));
    for (int i = 0; i < context.sources.size(); ++i) {
        const QVariantMap source = context.sources.at(i).toMap();
        const int id = source.value(QStringLiteral("id")).toInt();
        const QString name = source.value(QStringLiteral("name")).toString();
        report.check(id >= 0, QStringLiteral("negative source id"));
        report.check(!ids.contains(id), QStringLiteral("duplicate source id"), QString::number(id));
        report.check(!name.isEmpty(), QStringLiteral("empty source name"), QString::number(id));
        if (i < context.sourceNames.size())
            report.check(context.sourceNames.at(i) == name,
                         QStringLiteral("source name mismatch"), QString::number(id));
        ids.insert(id);
        report.check(declaredIds.contains(id), QStringLiteral("sourceIds missing source"),
                     QString::number(id));
        if (context.registry)
            report.check(context.registry->contains(id),
                         QStringLiteral("source not registered"), QString::number(id));
    }
    report.check(ids == declaredIds, QStringLiteral("sourceIds/source list mismatch"));

    if (context.registry) {
        for (int builtin : {ApiSource::Kugou, ApiSource::Netease, ApiSource::Bilibili,
                            ApiSource::QQMusic})
            report.check(context.registry->source(builtin) != nullptr,
                         QStringLiteral("builtin source missing"), QString::number(builtin));

        if (context.currentSource >= 0 && context.currentSource < ApiSource::PluginBase)
            report.check(context.registry->contains(context.currentSource),
                         QStringLiteral("current source not registered"),
                         QString::number(context.currentSource));
    }

    for (const QVariant &value : context.plugins) {
        const QVariantMap plugin = value.toMap();
        const int id = plugin.value(QStringLiteral("id")).toInt();
        report.check(id >= ApiSource::PluginBase, QStringLiteral("invalid plugin id"),
                     QString::number(id));
        report.check(!ApiSource::isBuiltin(id), QStringLiteral("plugin id collides with builtin"),
                     QString::number(id));
        report.check(context.registry && context.registry->contains(id),
                     QStringLiteral("plugin id not registered"), QString::number(id));
    }

    report.check(context.pendingRequests >= 0, QStringLiteral("negative pending request count"));
    report.check(context.lyricTimesSorted, QStringLiteral("lyric timeline is not sorted"));
}

void checkNormalization(Report &report)
{
    const QVariantMap item = ApiCommon::normalizeItem(
        QVariantMap{{QStringLiteral("name"), QStringLiteral("title")},
                    {QStringLiteral("singer"), QStringLiteral("artist")},
                    {QStringLiteral("pic"), QStringLiteral("cover")},
                    {QStringLiteral("id"), QStringLiteral("hash")}});
    report.check(item.value(ApiCommon::kTitle).toString() == QStringLiteral("title"),
                 QStringLiteral("normalize title"));
    report.check(item.value(ApiCommon::kArtist).toString() == QStringLiteral("artist"),
                 QStringLiteral("normalize artist"));
    report.check(item.value(ApiCommon::kCover).toString() == QStringLiteral("cover"),
                 QStringLiteral("normalize cover"));
    report.check(item.value(ApiCommon::kHash).toString() == QStringLiteral("hash"),
                 QStringLiteral("normalize hash"));
    report.check(item.value(ApiCommon::kHashHq).toString() == QStringLiteral("hash"),
                 QStringLiteral("normalize hashhq fallback"));
}

void checkModel(Report &report)
{
    OnlineListModel model;
    model.append(QVariantList{
        QVariantMap{{QStringLiteral("title"), QStringLiteral("a")},
                    {QStringLiteral("hash"), QStringLiteral("h1")},
                    {QStringLiteral("extra"), QStringLiteral("x")}},
        QVariantMap{{QStringLiteral("title"), QStringLiteral("b")},
                    {QStringLiteral("artist"), QStringLiteral("c")}}
    });
    report.check(model.rowCount() == 2, QStringLiteral("model append count"));
    const auto roles = model.roleNames();
    const int titleRole = roles.key(QByteArrayLiteral("title"));
    report.check(titleRole > 0, QStringLiteral("model title role missing"));
    report.check(model.data(model.index(1), titleRole).toString() == QStringLiteral("b"),
                 QStringLiteral("model data lookup"));
    report.check(model.get(0).value(QStringLiteral("extra")).toString() == QStringLiteral("x"),
                 QStringLiteral("model dynamic column"));
    model.remove(0, 1);
    report.check(model.rowCount() == 1, QStringLiteral("model remove count"));
    model.clear();
    report.check(model.rowCount() == 0, QStringLiteral("model clear count"));
}

void checkRouter(const SelfCheckContext &context, Report &report)
{
    if (!context.qualityCache) {
        report.check(false, QStringLiteral("quality cache missing"));
        return;
    }

    OnlineListModel songs;
    OnlineListModel menu;
    OnlineListModel comments;
    OnlineModels models;
    models.playlistSongs = &songs;
    models.playlistMenu = &menu;
    models.comments = &comments;

    ApiResultRouter router(models, context.qualityCache);
    router.setSongFilter(1);

    bool hasMore = false;
    QObject::connect(&router, &ApiResultRouter::commentsHasMoreChanged,
                     [&hasMore](bool value) { hasMore = value; });

    const QVariantList tracks{
        QVariantMap{{QStringLiteral("title"), QStringLiteral("free")},
                    {QStringLiteral("hash"), QStringLiteral("free-hash")},
                    {QStringLiteral("paytype"), ApiCommon::kPaytypeFree}},
        QVariantMap{{QStringLiteral("title"), QStringLiteral("vip")},
                    {QStringLiteral("hash"), QStringLiteral("vip-hash")},
                    {QStringLiteral("paytype"), ApiCommon::kPaytypePaid}}
    };
    report.check(router.route(ApiAction::PlaylistSongs,
                              QVariantMap{{QStringLiteral("info"), tracks}}, 0),
                 QStringLiteral("router playlist route"));
    report.check(songs.rowCount() == 1, QStringLiteral("router paytype filter"),
                 QString::number(songs.rowCount()));

    router.route(ApiAction::PlaylistMenu,
                 QVariantMap{{QStringLiteral("info"), QVariantList{
                     QVariantMap{{QStringLiteral("title"), QStringLiteral("menu")},
                                 {QStringLiteral("category"), QStringLiteral("cat")},
                                 {QStringLiteral("id"), 7}}}}},
                 0);
    report.check(menu.rowCount() == 1, QStringLiteral("router menu size"));
    report.check(menu.rowCount() == 0
                     || menu.get(0).value(QStringLiteral("id")).toInt() == 7,
                 QStringLiteral("router menu id"));

    router.route(ApiAction::Comments,
                 QVariantMap{{QStringLiteral("info"), QVariantList{
                     QVariantMap{{QStringLiteral("user"), QStringLiteral("u")},
                                 {QStringLiteral("content"), QStringLiteral("c")}}}},
                             {QStringLiteral("hasMore"), true}},
                 0);
    report.check(comments.rowCount() == 1, QStringLiteral("router comments count"));
    report.check(hasMore, QStringLiteral("router comments hasMore"));
    report.check(!router.route(QStringLiteral("unknownAction"), {}, 0),
                 QStringLiteral("router unknown action should fail"));
}

void checkDispatch(Report &report)
{
    ProbeSource source;
    QString action;
    QVariantMap payload;
    int resultSource = -1;
    QObject::connect(&source, &MusicSource::resultReady,
                     [&](const QString &value, const QVariant &data, int id) {
                         action = value;
                         payload = data.toMap();
                         resultSource = id;
                     });

    source.request(9999, ApiAction::SearchSongs,
                   QVariantMap{{ApiArg::Keyword, QStringLiteral("k")},
                               {ApiArg::Type, 1},
                               {ApiArg::Page, 2},
                               {ApiArg::PageSize, 9}});
    report.check(action == ApiAction::SearchSongs, QStringLiteral("dispatch action"));
    report.check(resultSource == 9999, QStringLiteral("dispatch source"));
    report.check(payload.value(QStringLiteral("keyword")).toString() == QStringLiteral("k"),
                 QStringLiteral("dispatch keyword"));
    report.check(payload.value(QStringLiteral("page")).toInt() == 2,
                 QStringLiteral("dispatch page"));

    action.clear();
    source.request(9999, QStringLiteral("unknownAction"), {});
    report.check(action == QStringLiteral("unknownAction"), QStringLiteral("dispatch unknown action"));
    report.check(payload.contains(QStringLiteral("error")),
                 QStringLiteral("dispatch unknown should return error"));
}

void checkLyrics(Report &report)
{
    report.check(LyricTimeline::indexAt({}, 0) == 0, QStringLiteral("lyric empty"));
    const QList<qlonglong> times{100, 200, 300};
    report.check(LyricTimeline::indexAt(times, -1) == 0, QStringLiteral("lyric before first"));
    report.check(LyricTimeline::indexAt(times, 100) == 0, QStringLiteral("lyric first"));
    report.check(LyricTimeline::indexAt(times, 150) == 0, QStringLiteral("lyric between"));
    report.check(LyricTimeline::indexAt(times, 200) == 1, QStringLiteral("lyric second"));
    report.check(LyricTimeline::indexAt(times, 999) == 2, QStringLiteral("lyric last"));
}

} // namespace

QVariantMap ApiSelfCheck::run(const SelfCheckContext &context)
{
    QElapsedTimer timer;
    timer.start();

    Report report;
    checkSourceList(context, report);
    checkNormalization(report);
    checkModel(report);
    checkRouter(context, report);
    checkDispatch(report);
    checkLyrics(report);

    if (context.qualityCache) {
        const QString probe = QStringLiteral("__quemusic_selfcheck__");
        for (int quality = 0; quality <= 3; ++quality)
            report.check(context.qualityCache->resolve(probe, quality) == probe,
                         QStringLiteral("quality cache passthrough"));
        report.check(context.qualityCache->baseOf(probe) == probe,
                     QStringLiteral("quality cache base passthrough"));
    } else {
        report.check(false, QStringLiteral("quality cache missing"));
    }

    QVariantMap result;
    result.insert(QStringLiteral("ok"), report.failed == 0);
    result.insert(QStringLiteral("passed"), report.passed);
    result.insert(QStringLiteral("failed"), report.failed);
    result.insert(QStringLiteral("issues"), report.issues);
    result.insert(QStringLiteral("elapsedMs"), int(timer.elapsed()));
    return result;
}
