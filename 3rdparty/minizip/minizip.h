// Copyright (C) 2004-2023 Robert Griebl
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QCoreApplication>
#include <QHash>

QT_FORWARD_DECLARE_CLASS(QIODevice)


class MiniZip
{
    Q_DECLARE_TR_FUNCTIONS(MiniZip)

public:
    MiniZip(const QString &zipFileName);
    ~MiniZip();

    bool open();
    void close();
    QStringList fileList() const;
    bool contains(const QString &fileName) const;
    QByteArray readFile(const QString &fileName);

    static void unzip(const QString &zipFileName, QIODevice *destination,
                      const char *extractFileName, const char *extractPassword = nullptr);

private:
    bool openInternal(bool parseTOC);

    QString m_zipFileName;
    QHash<QByteArray, QPair<quint64, quint64>> m_contents;
    void *m_zip = nullptr;

};
