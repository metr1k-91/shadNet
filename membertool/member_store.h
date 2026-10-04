// SPDX-FileCopyrightText: Copyright 2026 shadNet Project
// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <optional>
#include <QList>
#include <QString>
#include "database.h"

struct Member {
    int64_t id = 0;
    QString username;
    QString email;
};

struct MemberCredentials {
    QString username;
    QString email;
    QString password;
    QString token;
};

class MemberStore {
public:
    bool Open(const QString& path, QString& error);
    bool IsOpen() const {
        return m_db != nullptr;
    }
    QString Path() const {
        return m_path;
    }
    bool NeedsToken() const;
    QList<Member> List(QString& error) const;
    std::optional<MemberCredentials> Add(const QString& username, QString& error);
    bool Remove(int64_t id, QString& error, QString& warning);

private:
    std::unique_ptr<Database> m_db;
    QString m_path;
};
