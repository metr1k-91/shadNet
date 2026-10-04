// SPDX-FileCopyrightText: Copyright 2026 shadNet Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include "member_window.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("shadNet");
    app.setApplicationName("Member Manager");

    QString path;
    if (app.arguments().size() > 1) {
        path = app.arguments().at(1);
    } else {
        const QDir executable(app.applicationDirPath());
        const QStringList candidates = {
            QSettings().value("databasePath").toString(),
            executable.filePath("db/shadnet.db"),
            executable.filePath("../db/shadnet.db"),
        };
        for (const QString& candidate : candidates) {
            if (QFileInfo(candidate).isFile()) {
                path = candidate;
                break;
            }
        }
    }
    MemberWindow window(path);
    window.show();
    return app.exec();
}
