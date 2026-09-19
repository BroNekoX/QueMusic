// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 QueMusic Contributors
//
// 本地文件批量删除的回归测试：删除在工作线程执行、逐个上报进度、
// 完成后只摘掉被删行（不重扫目录 —— 旧实现卡顿的主因之一）
#include <QtTest>

#include "LocalMusicScanner.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUrl>

class LocalMusicDeleteTest : public QObject
{
    Q_OBJECT

private slots:
    void deletesInWorkerThreadWithProgressAndKeepsOthers();
};

static QStringList createFiles(const QString &dir, const QStringList &names)
{
    QStringList paths;
    for (const QString &name : names) {
        const QString path = dir + QLatin1Char('/') + name;
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            continue;
        file.write(QByteArray(64, 'Q'));
        file.close();
        paths.append(path);
    }
    return paths;
}

void LocalMusicDeleteTest::deletesInWorkerThreadWithProgressAndKeepsOthers()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    const QStringList all = createFiles(tmp.path(), {"a.mp3", "b.mp3", "c.mp3", "d.mp3", "e.mp3"});
    QCOMPARE(all.size(), 5);

    LocalMusicScanner scanner;
    QVERIFY(!scanner.deleting());

    QSignalSpy scanSpy(&scanner, &LocalMusicScanner::scanFinished);
    scanner.setFolder(QUrl::fromLocalFile(tmp.path()));
    QTRY_VERIFY_WITH_TIMEOUT(scanSpy.count() > 0, 30000);
    QCOMPARE(scanner.rowCount(), 5);

    const QVariantList targets{all.at(0), all.at(1), all.at(2)};
    QSignalSpy progressSpy(&scanner, &LocalMusicScanner::deleteProgress);
    QSignalSpy deleteSpy(&scanner, &LocalMusicScanner::deleteFinished);

    scanner.deleteFiles(targets);

    // 立即上报总量（0/N），界面才能马上显示进度条
    QCOMPARE(progressSpy.count(), 1);
    QCOMPARE(progressSpy.at(0).at(0).toInt(), 0);
    QCOMPARE(progressSpy.at(0).at(1).toInt(), targets.size());
    QVERIFY(scanner.deleting());

    QTRY_VERIFY_WITH_TIMEOUT(deleteSpy.count() == 1, 60000);

    const int removed = deleteSpy.at(0).at(0).toInt();
    const int failed = deleteSpy.at(0).at(1).toInt();
    // 成功 + 失败必须等于总数，不允许漏报
    QCOMPARE(removed + failed, targets.size());

    // 进度逐条上报，最后一帧是 N/N
    QCOMPARE(progressSpy.count(), 1 + targets.size());
    QCOMPARE(progressSpy.last().at(0).toInt(), targets.size());
    QCOMPARE(progressSpy.last().at(1).toInt(), targets.size());

    // 报告删除的文件必须真的不在磁盘上
    int gone = 0;
    for (const QVariant &path : targets) {
        if (!QFile::exists(path.toString()))
            ++gone;
    }
    QCOMPARE(gone, removed);

    // 未选中的文件不受影响
    QVERIFY(QFile::exists(all.at(3)));
    QVERIFY(QFile::exists(all.at(4)));

    // 只摘掉对应行，且不得触发整目录重扫
    QCOMPARE(scanner.rowCount(), 5 - removed);
    QCOMPARE(scanSpy.count(), 1);

    // 状态复位，否则界面上的进度框关不掉
    QVERIFY(!scanner.deleting());
}

// GUILESS：逻辑不需要窗口，无显示环境（CI/容器）也能跑
QTEST_GUILESS_MAIN(LocalMusicDeleteTest)
#include "local_music_delete_test.moc"
