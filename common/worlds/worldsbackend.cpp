// SPDX-FileCopyrightText: Copyright 2026 shadNet Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "worldsbackend.h"

DemoWorldsBackend::DemoWorldsBackend(QObject* parent) : WorldsBackend(parent) {
    const QString content = serializeWorldsConfig(parseWorldsConfig(exampleWorldsConfig()));
    m_snapshot = {content, QStringLiteral("r1"), content, QStringLiteral("r1"), true, true};
}
void DemoWorldsBackend::load() {
    emit received(m_snapshot);
}
void DemoWorldsBackend::save(const QString& content, const QString& revision, bool apply) {
    if (revision != m_snapshot.revision) {
        emit failed(tr("The saved configuration changed. Read it again."), true);
        return;
    }
    if (content != m_snapshot.content) {
        m_snapshot.content = content;
        m_snapshot.revision = QStringLiteral("r%1").arg(++m_revision);
    }
    if (apply) {
        m_snapshot.activeContent = content;
        m_snapshot.activeRevision = m_snapshot.revision;
    }
    emit received(m_snapshot);
}
void DemoWorldsBackend::reload(const QString& revision) {
    save(m_snapshot.content, revision, true);
}
