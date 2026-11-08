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
        issues.append(detail.isEmpty() ? name : name + ": " + detail);
    }

    int passed = 0;
    int failed = 0;
    QStringList issues;
};

class ProbeSource : public MusicSource
{
public:
    ProbeSource() : MusicSource(9999) {}
    QString sourceName() const override { return "probe"; }

protected:
    void onSearchSongs(const QString &keyword, int type, int page, int pageSize) override
    {
        deliver(ApiAction::SearchSongs,
                QVariantMap{{"keyword", keyword},
                            {"type", type},
                            {"page", page},
                            {"pageSize", pageSize}},
                9999);
    }
};

void checkSourceList(const SelfCheckContext &context, Report &report)
{
    report.check(!context.sources.isEmpty(), "source list is empty");
    report.check(context.sourceIds.size() == context.sources.size(),
                 "sourceIds size mismatch");
    report.check(context.sourceNames.size() == context.sources.size(),
                 "sourceNames size mismatch");

    QSet<int> ids;
    QSet<int> declaredIds;
    for (const QVariant &value : context.sourceIds)
        declaredIds.insert(value.toInt());
    report.check(declaredIds.size() == context.sourceIds.size(),
                 "duplicate sourceIds");
    for (int i = 0; i < context.sources.size(); ++i) {
        const QVariantMap source = context.sources.at(i).toMap();
        const int id = source.value("id").toInt();
        const QString name = source.value("name").toString();
        report.check(id >= 0, "negative source id");
        report.check(!ids.contains(id), "duplicate source id", QString::number(id));
        report.check(!name.isEmpty(), "empty source name", QString::number(id));
        if (i < context.sourceNames.size())
            report.check(context.sourceNames.at(i) == name,
                         "source name mismatch", QString::number(id));
        ids.insert(id);
        report.check(declaredIds.contains(id), "sourceIds missing source",
                     QString::number(id));
        if (context.registry)
            report.check(context.registry->contains(id),
                         "source not registered", QString::number(id));
    }
    report.check(ids == declaredIds, "sourceIds/source list mismatch");

    if (context.registry) {
        for (int builtin : {ApiSource::Kugou, ApiSource::Netease, ApiSource::Bilibili,
                            ApiSource::QQMusic, ApiSource::Spotify, ApiSource::AppleMusic})
            report.check(context.registry->source(builtin) != nullptr,
                         "builtin source missing", QString::number(builtin));
        if (auto *source = context.registry->source(ApiSource::Spotify))
            report.check(source->sourceName() == "Spotify", "spotify source name");
        if (auto *source = context.registry->source(ApiSource::AppleMusic))
            report.check(source->sourceName() == "Apple Music", "apple source name");

        if (context.currentSource >= 0 && context.currentSource < ApiSource::PluginBase)
            report.check(context.registry->contains(context.currentSource),
                         "current source not registered",
                         QString::number(context.currentSource));
    }

    for (const QVariant &value : context.plugins) {
        const QVariantMap plugin = value.toMap();
        const int id = plugin.value("id").toInt();
        report.check(id >= ApiSource::PluginBase, "invalid plugin id",
                     QString::number(id));
        report.check(!ApiSource::isBuiltin(id), "plugin id collides with builtin",
                     QString::number(id));
        report.check(context.registry && context.registry->contains(id),
                     "plugin id not registered", QString::number(id));
    }

    report.check(context.pendingRequests >= 0, "negative pending request count");
    report.check(context.lyricTimesSorted, "lyric timeline is not sorted");
}

void checkNormalization(Report &report)
{
    const QVariantMap item = ApiCommon::normalizeItem(
        QVariantMap{{"name", "title"},
                    {"singer", "artist"},
                    {"pic", "cover"},
                    {"id", "hash"}});
    report.check(item.value(ApiCommon::kTitle).toString() == "title",
                 "normalize title");
    report.check(item.value(ApiCommon::kArtist).toString() == "artist",
                 "normalize artist");
    report.check(item.value(ApiCommon::kCover).toString() == "cover",
                 "normalize cover");
    report.check(item.value(ApiCommon::kHash).toString() == "hash",
                 "normalize hash");
    report.check(item.value(ApiCommon::kHashHq).toString() == "hash",
                 "normalize hashhq fallback");
}

void checkModel(Report &report)
{
    OnlineListModel model;
    model.append(QVariantList{
        QVariantMap{{"title", "a"},
                    {"hash", "h1"},
                    {"extra", "x"}},
        QVariantMap{{"title", "b"},
                    {"artist", "c"}}
    });
    report.check(model.rowCount() == 2, "model append count");
    const auto roles = model.roleNames();
    const int titleRole = roles.key(QByteArrayLiteral("title"));
    report.check(titleRole > 0, "model title role missing");
    report.check(model.data(model.index(1), titleRole).toString() == "b",
                 "model data lookup");
    report.check(model.get(0).value("extra").toString() == "x",
                 "model dynamic column");
    model.remove(0, 1);
    report.check(model.rowCount() == 1, "model remove count");
    model.clear();
    report.check(model.rowCount() == 0, "model clear count");
}

void checkRouter(const SelfCheckContext &context, Report &report)
{
    if (!context.qualityCache) {
        report.check(false, "quality cache missing");
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
        QVariantMap{{"title", "free"},
                    {"hash", "free-hash"},
                    {"paytype", ApiCommon::kPaytypeFree}},
        QVariantMap{{"title", "vip"},
                    {"hash", "vip-hash"},
                    {"paytype", ApiCommon::kPaytypePaid}}
    };
    report.check(router.route(ApiAction::PlaylistSongs,
                              QVariantMap{{"info", tracks}}, 0),
                 "router playlist route");
    report.check(songs.rowCount() == 1, "router paytype filter",
                 QString::number(songs.rowCount()));

    router.route(ApiAction::PlaylistMenu,
                 QVariantMap{{"info", QVariantList{
                     QVariantMap{{"title", "menu"},
                                 {"category", "cat"},
                                 {"id", 7}}}}},
                 0);
    report.check(menu.rowCount() == 1, "router menu size");
    report.check(menu.rowCount() == 0
                     || menu.get(0).value("id").toInt() == 7,
                 "router menu id");

    router.route(ApiAction::Comments,
                 QVariantMap{{"info", QVariantList{
                     QVariantMap{{"user", "u"},
                                 {"content", "c"}}}},
                             {"hasMore", true}},
                 0);
    report.check(comments.rowCount() == 1, "router comments count");
    report.check(hasMore, "router comments hasMore");
    report.check(!router.route("unknownAction", {}, 0),
                 "router unknown action should fail");
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
                   QVariantMap{{ApiArg::Keyword, "k"},
                               {ApiArg::Type, 1},
                               {ApiArg::Page, 2},
                               {ApiArg::PageSize, 9}});
    report.check(action == ApiAction::SearchSongs, "dispatch action");
    report.check(resultSource == 9999, "dispatch source");
    report.check(payload.value("keyword").toString() == "k",
                 "dispatch keyword");
    report.check(payload.value("page").toInt() == 2,
                 "dispatch page");

    action.clear();
    source.request(9999, "unknownAction", {});
    report.check(action == "unknownAction", "dispatch unknown action");
    report.check(payload.contains("error"),
                 "dispatch unknown should return error");
}

void checkLyrics(Report &report)
{
    report.check(LyricTimeline::indexAt({}, 0) == 0, "lyric empty");
    const QList<qlonglong> times{100, 200, 300};
    report.check(LyricTimeline::indexAt(times, -1) == 0, "lyric before first");
    report.check(LyricTimeline::indexAt(times, 100) == 0, "lyric first");
    report.check(LyricTimeline::indexAt(times, 150) == 0, "lyric between");
    report.check(LyricTimeline::indexAt(times, 200) == 1, "lyric second");
    report.check(LyricTimeline::indexAt(times, 999) == 2, "lyric last");
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
        const QString probe = "__quemusic_selfcheck__";
        for (int quality = 0; quality <= 3; ++quality)
            report.check(context.qualityCache->resolve(probe, quality) == probe,
                         "quality cache passthrough");
        report.check(context.qualityCache->baseOf(probe) == probe,
                     "quality cache base passthrough");
    } else {
        report.check(false, "quality cache missing");
    }

    QVariantMap result;
    result.insert("ok", report.failed == 0);
    result.insert("passed", report.passed);
    result.insert("failed", report.failed);
    result.insert("issues", report.issues);
    result.insert("elapsedMs", int(timer.elapsed()));
    return result;
}
