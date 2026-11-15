// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2025-2026 QueMusic Contributors
#include "plugins/ZipReader.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QtEndian>

#include <QtZlib/zlib.h>

namespace {

quint16 u16(const QByteArray &d, int p)
{
    if (p < 0 || p + 2 > d.size())
        return 0;
    return qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(d.constData() + p));
}

quint32 u32(const QByteArray &d, int p)
{
    if (p < 0 || p + 4 > d.size())
        return 0;
    return qFromLittleEndian<quint32>(reinterpret_cast<const uchar *>(d.constData() + p));
}

bool safeRelPath(const QString &name)
{
    if (name.isEmpty() || name.startsWith(QLatin1Char('/')) || name.contains(QLatin1Char('\\')))
        return false;
    if (name.size() > 1 && name.at(1) == QLatin1Char(':'))
        return false;
    const QStringList parts = name.split(QLatin1Char('/'));
    for (const QString &part : parts) {
        if (part == QStringLiteral(".."))
            return false;
    }
    return true;
}

QByteArray inflateRaw(const QByteArray &in, int expected)
{
    QByteArray out(qMax(expected, 1), Qt::Uninitialized);
    z_stream zs{};
    if (inflateInit2(&zs, -MAX_WBITS) != Z_OK)
        return {};
    zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(in.constData()));
    zs.avail_in = uInt(in.size());
    zs.next_out = reinterpret_cast<Bytef *>(out.data());
    zs.avail_out = uInt(out.size());
    const int ret = inflate(&zs, Z_FINISH);
    const int produced = int(zs.total_out);
    inflateEnd(&zs);
    if (ret != Z_STREAM_END)
        return {};
    out.resize(produced);
    return out;
}

} // namespace

bool ZipReader::extract(const QByteArray &zip, const QString &destDir, QString *error)
{
    const auto fail = [error](const QString &text) {
        if (error)
            *error = text;
        return false;
    };

    int eocd = -1;
    const int searchFrom = qMax(0, zip.size() - 65557);
    for (int i = zip.size() - 22; i >= searchFrom; --i) {
        if (u32(zip, i) == 0x06054b50) {
            eocd = i;
            break;
        }
    }
    if (eocd < 0)
        return fail(QStringLiteral("不是有效的 zip"));

    const int count = u16(zip, eocd + 10);
    int pos = int(u32(zip, eocd + 16));
    if (count <= 0 || pos <= 0)
        return fail(QStringLiteral("zip 目录为空"));

    QDir root(destDir);
    if (!root.mkpath(QStringLiteral(".")))
        return fail(QStringLiteral("无法创建解压目录"));

    for (int n = 0; n < count; ++n) {
        if (u32(zip, pos) != 0x02014b50)
            return fail(QStringLiteral("zip 中央目录损坏"));
        const int method = u16(zip, pos + 10);
        const quint32 compSize = u32(zip, pos + 20);
        const quint32 rawSize = u32(zip, pos + 24);
        const int nameLen = u16(zip, pos + 28);
        const int extraLen = u16(zip, pos + 30);
        const int commentLen = u16(zip, pos + 32);
        const int localPos = int(u32(zip, pos + 42));
        if (compSize == 0xFFFFFFFFu || rawSize == 0xFFFFFFFFu)
            return fail(QStringLiteral("不支持 zip64"));
        if (pos + 46 + nameLen > zip.size())
            return fail(QStringLiteral("zip 目录越界"));

        const QString name = QString::fromUtf8(zip.constData() + pos + 46, nameLen);
        pos += 46 + nameLen + extraLen + commentLen;
        if (!safeRelPath(name))
            return fail(QStringLiteral("zip 含非法路径: %1").arg(name));

        const QString target = root.absoluteFilePath(name);
        if (name.endsWith(QLatin1Char('/'))) {
            root.mkpath(name);
            continue;
        }
        if (u32(zip, localPos) != 0x04034b50)
            return fail(QStringLiteral("zip 本地头损坏"));
        const int localName = u16(zip, localPos + 26);
        const int localExtra = u16(zip, localPos + 28);
        const int dataPos = localPos + 30 + localName + localExtra;
        if (dataPos + int(compSize) > zip.size())
            return fail(QStringLiteral("zip 数据越界"));

        const QByteArray raw = zip.mid(dataPos, int(compSize));
        QByteArray data;
        if (method == 0) {
            data = raw;
        } else if (method == 8) {
            data = inflateRaw(raw, int(rawSize));
            if (data.size() != int(rawSize))
                return fail(QStringLiteral("zip 解压失败: %1").arg(name));
        } else {
            return fail(QStringLiteral("不支持的压缩方式 %1").arg(method));
        }

        QDir().mkpath(QFileInfo(target).absolutePath());
        QFile file(target);
        if (!file.open(QIODevice::WriteOnly) || file.write(data) != data.size())
            return fail(QStringLiteral("写入失败: %1").arg(name));
    }
    return true;
}