#include <QtTest>

#include "LocalLyricsReader.h"

class LocalLyricsReaderTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesLrcTimestampsAndSorts();
    void parsesEnhancedLrcWordTimings();
    void stripsEmptyWordLabelsFromText();
    void readsSidecarBeforeEmbeddedLyrics();
    void readsEmbeddedId3LyricsWhenSidecarIsMissing();
};

static QByteArray syncSafeSize(int size)
{
    QByteArray encoded(4, '\0');
    encoded[0] = char((size >> 21) & 0x7f);
    encoded[1] = char((size >> 14) & 0x7f);
    encoded[2] = char((size >> 7) & 0x7f);
    encoded[3] = char(size & 0x7f);
    return encoded;
}

static QByteArray id3v23Uslt(const QByteArray &text)
{
    QByteArray payload;
    payload.append(char(0));             // ISO-8859-1 text encoding
    payload.append("eng", 3);
    payload.append(char(0));             // empty description
    payload.append(text);

    QByteArray frame("USLT", 4);
    frame.append(char((payload.size() >> 24) & 0xff));
    frame.append(char((payload.size() >> 16) & 0xff));
    frame.append(char((payload.size() >> 8) & 0xff));
    frame.append(char(payload.size() & 0xff));
    frame.append("\0\0", 2);
    frame.append(payload);

    QByteArray id3("ID3", 3);
    id3.append(char(3));
    id3.append(char(0));
    id3.append(char(0));
    id3.append(syncSafeSize(frame.size()));
    id3.append(frame);
    return id3;
}

void LocalLyricsReaderTest::parsesLrcTimestampsAndSorts()
{
    const QVariantList lyrics = LocalLyricsReader::parseLrc(
        QStringLiteral("[ar:测试]\n[00:03.50]晚一点\n[00:01.2][00:02.00]前奏\n"));

    QCOMPARE(lyrics.size(), 3);
    QCOMPARE(lyrics.at(0).toMap().value(QStringLiteral("time")).toLongLong(), 1200LL);
    QCOMPARE(lyrics.at(1).toMap().value(QStringLiteral("time")).toLongLong(), 2000LL);
    QCOMPARE(lyrics.at(2).toMap().value(QStringLiteral("time")).toLongLong(), 3500LL);
    QCOMPARE(lyrics.at(0).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("前奏"));
}

void LocalLyricsReaderTest::parsesEnhancedLrcWordTimings()
{
    const QVariantList lyrics = LocalLyricsReader::parseLrc(
        QStringLiteral("[00:10.00]<00:10.00>我 <00:10.50>爱 <00:11.00>你\n"
                       "[00:12.00]结尾\n"
                       "[00:14.00]<0,300,0>起 <300,400,0>风\n"));

    QCOMPARE(lyrics.size(), 3);

    // 增强 LRC：绝对字级时间 → 相对本行的 offset/duration，行尾字沿用上一字时长
    const QVariantMap enhanced = lyrics.at(0).toMap();
    QCOMPARE(enhanced.value(QStringLiteral("time")).toLongLong(), 10000LL);
    QCOMPARE(enhanced.value(QStringLiteral("text")).toString(), QStringLiteral("我 爱 你"));
    const QVariantList info = enhanced.value(QStringLiteral("info")).toList();
    QCOMPARE(info.size(), 3);
    QCOMPARE(info.at(0).toMap().value(QStringLiteral("offset")).toLongLong(), 0LL);
    QCOMPARE(info.at(1).toMap().value(QStringLiteral("offset")).toLongLong(), 500LL);
    QCOMPARE(info.at(2).toMap().value(QStringLiteral("offset")).toLongLong(), 1000LL);
    QCOMPARE(info.at(0).toMap().value(QStringLiteral("duration")).toLongLong(), 500LL);
    QCOMPARE(info.at(1).toMap().value(QStringLiteral("duration")).toLongLong(), 500LL);
    QCOMPARE(info.at(2).toMap().value(QStringLiteral("duration")).toLongLong(), 500LL);

    // 无字级标签的普通行保持 null info，走整行高亮
    const QVariantMap plain = lyrics.at(1).toMap();
    QCOMPARE(plain.value(QStringLiteral("text")).toString(), QStringLiteral("结尾"));
    QVERIFY(!plain.value(QStringLiteral("info")).isValid());

    // KRC 风格相对字级标签，沿用在线逐字解析
    const QVariantMap krcStyle = lyrics.at(2).toMap();
    QCOMPARE(krcStyle.value(QStringLiteral("time")).toLongLong(), 14000LL);
    QCOMPARE(krcStyle.value(QStringLiteral("text")).toString(), QStringLiteral("起 风"));
    const QVariantList krcInfo = krcStyle.value(QStringLiteral("info")).toList();
    QCOMPARE(krcInfo.at(0).toMap().value(QStringLiteral("offset")).toLongLong(), 0LL);
    QCOMPARE(krcInfo.at(0).toMap().value(QStringLiteral("duration")).toLongLong(), 300LL);
    QCOMPARE(krcInfo.at(1).toMap().value(QStringLiteral("offset")).toLongLong(), 300LL);
    QCOMPARE(krcInfo.at(1).toMap().value(QStringLiteral("duration")).toLongLong(), 400LL);
}

void LocalLyricsReaderTest::stripsEmptyWordLabelsFromText()
{
    const QVariantList lyrics = LocalLyricsReader::parseLrc(
        QStringLiteral("[00:09.00]前言<00:00.000>\n"
                       "[00:10.00]正文\n"
                       "[00:11.00]<00:00.000><00:11.20>尾奏\n"));

    QCOMPARE(lyrics.size(), 3);
    for (const QVariant &value : lyrics) {
        const QString text = value.toMap().value(QStringLiteral("text")).toString();
        QVERIFY2(!text.contains(QLatin1Char('<')), qPrintable(QStringLiteral("残留 '<' : ") + text));
        QVERIFY2(!text.contains(QLatin1Char('>')), qPrintable(QStringLiteral("残留 '>' : ") + text));
    }
    // 无名文本的空标签不应成为首词或被塞进浮层文本
    QCOMPARE(lyrics.at(0).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("前言"));
    QCOMPARE(lyrics.at(2).toMap().value(QStringLiteral("text")).toString(), QStringLiteral("尾奏"));
    const QVariantList lastInfo = lyrics.at(2).toMap().value(QStringLiteral("info")).toList();
    QCOMPARE(lastInfo.size(), 1);
    QCOMPARE(lastInfo.at(0).toMap().value(QStringLiteral("offset")).toLongLong(), 200LL);
}

void LocalLyricsReaderTest::readsSidecarBeforeEmbeddedLyrics()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString audioPath = dir.filePath(QStringLiteral("song.mp3"));
    QFile audio(audioPath);
    QVERIFY(audio.open(QIODevice::WriteOnly));
    const QByteArray embedded = id3v23Uslt("[00:02.00]embedded");
    QVERIFY(audio.write(embedded) == embedded.size());
    audio.close();

    QFile lrc(dir.filePath(QStringLiteral("song.lrc")));
    QVERIFY(lrc.open(QIODevice::WriteOnly | QIODevice::Text));
    QVERIFY(lrc.write("[00:01.00]sidecar\n") > 0);
    lrc.close();

    const QVariantMap result = LocalLyricsReader::read(audioPath);
    QCOMPARE(result.value(QStringLiteral("source")).toString(), QStringLiteral("sidecar"));
    QCOMPARE(result.value(QStringLiteral("found")).toBool(), true);
    const QVariantList lyrics = result.value(QStringLiteral("lyrics")).toList();
    QCOMPARE(lyrics.size(), 1);
    QCOMPARE(lyrics.first().toMap().value(QStringLiteral("text")).toString(), QStringLiteral("sidecar"));
}

void LocalLyricsReaderTest::readsEmbeddedId3LyricsWhenSidecarIsMissing()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString audioPath = dir.filePath(QStringLiteral("embedded.mp3"));

    QFile audio(audioPath);
    QVERIFY(audio.open(QIODevice::WriteOnly));
    const QByteArray id3 = id3v23Uslt("[00:02.00]embedded");
    QVERIFY(audio.write(id3) == id3.size());
    audio.close();

    const QVariantMap result = LocalLyricsReader::read(audioPath);
    QCOMPARE(result.value(QStringLiteral("source")).toString(), QStringLiteral("embedded"));
    QCOMPARE(result.value(QStringLiteral("found")).toBool(), true);
    const QVariantList lyrics = result.value(QStringLiteral("lyrics")).toList();
    QCOMPARE(lyrics.size(), 1);
    QCOMPARE(lyrics.first().toMap().value(QStringLiteral("time")).toLongLong(), 2000LL);
    QCOMPARE(lyrics.first().toMap().value(QStringLiteral("text")).toString(), QStringLiteral("embedded"));
}

QTEST_MAIN(LocalLyricsReaderTest)
#include "local_lyrics_reader_test.moc"
