// SPDX-FileCopyrightText: Copyright 2026 shadNet Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "member_window.h"

#include <QApplication>
#include <QClipboard>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

class MemberWindowTest : public QObject {
    Q_OBJECT
private slots:
    void createCopyFindAndRemove() {
        QTemporaryDir directory;
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, directory.path());
        QCoreApplication::setOrganizationName("shadNetTest");
        QCoreApplication::setApplicationName("MemberWindowTest");
        Database database{QString()};
        const QString path = directory.filePath("db/shadnet.db");
        QVERIFY(database.Open(path));

        MemberWindow window(path);
        window.show();
        auto* username = window.findChild<QLineEdit*>("username");
        auto* email = window.findChild<QLineEdit*>("email");
        auto* create = window.findChild<QPushButton*>("createMember");
        auto* credentials = window.findChild<QPlainTextEdit*>("credentials");
        auto* copy = window.findChild<QPushButton*>("copyCredentials");
        auto* search = window.findChild<QLineEdit*>("memberSearch");
        auto* members = window.findChild<QTableWidget*>("members");
        auto* remove = window.findChild<QPushButton*>("removeMember");
        QVERIFY(username && email && create && credentials && copy && search && members && remove);
        QVERIFY(!create->isEnabled());
        QVERIFY(!remove->isEnabled());
        QTest::keyClicks(username, "Alice");
        QCOMPARE(email->text(), "Alice@shadps4.local");
        QTest::mouseClick(create, Qt::LeftButton);
        QCOMPARE(members->rowCount(), 1);
        const auto match =
            QRegularExpression("Password: ([^\\n]+)").match(credentials->toPlainText());
        QVERIFY(match.hasMatch());
        QVERIFY(database.CheckUser("Alice", match.captured(1), {}, false));
        QTest::mouseClick(copy, Qt::LeftButton);
        QCOMPARE(QApplication::clipboard()->text(), credentials->toPlainText());

        const QString screenshot = qEnvironmentVariable("MEMBER_TOOL_SCREENSHOT");
        if (!screenshot.isEmpty()) {
            QCoreApplication::processEvents();
            QVERIFY(window.grab().save(screenshot));
        }
        search->setText("missing");
        QVERIFY(members->isRowHidden(0));
        search->setText("ALICE");
        QVERIFY(!members->isRowHidden(0));
        members->selectRow(0);
        QVERIFY(remove->isEnabled());

        // Cancelling the confirmation leaves the account intact.
        QTimer::singleShot(0, &window, [] {
            auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (dialog)
                dialog->button(QMessageBox::No)->click();
        });
        QTest::mouseClick(remove, Qt::LeftButton);
        QVERIFY(database.GetUserId("Alice"));
        QTimer::singleShot(0, &window, [] {
            auto* dialog = qobject_cast<QMessageBox*>(QApplication::activeModalWidget());
            if (dialog)
                dialog->button(QMessageBox::Yes)->click();
        });
        QTest::mouseClick(remove, Qt::LeftButton);
        QVERIFY(!database.GetUserId("Alice"));
        QCOMPARE(members->rowCount(), 0);
        QVERIFY(credentials->toPlainText().isEmpty());
        QVERIFY(!copy->isEnabled());
        QVERIFY(!remove->isEnabled());
    }
};

QTEST_MAIN(MemberWindowTest)
#include "member_window_test.moc"
