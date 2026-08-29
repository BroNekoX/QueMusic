#include <QtTest>

#include "LocalLyricsReader.h"

class LocalLyricsReaderTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesLrcTimestampsAndSorts();
    void parsesEnhancedLrcWordTimings();
    void stripsEmptyWordLabelsFromText();
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
