// SPDX-FileCopyrightText: Copyright 2026 shadNet Project
// SPDX-License-Identifier: GPL-2.0-or-later
#include "member_store.h"

#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QTemporaryDir>
#include <QtTest>

namespace {

struct Fixture {
    QTemporaryDir directory;
    Database database{QString()};
    MemberStore store;
    QString error;

    QString Path() const {
        return directory.filePath("db/shadnet.db");
    }
    bool Open() {
        return database.Open(Path()) && store.Open(Path(), error);
    }
};

bool Exec(Database& database, const QString& sql, const QVariantList& values = {}) {
    QSqlQuery query(database.Conn());
    if (!query.prepare(sql))
        return false;
    for (const auto& value : values)
        query.addBindValue(value);
    if (!query.exec()) {
        qWarning() << query.lastError();
        return false;
    }
    return true;
}

int Count(Database& database, const QString& table) {
    QSqlQuery query(database.Conn());
    if (!query.exec("SELECT COUNT(*) FROM " + table) || !query.next())
        return -1;
    return query.value(0).toInt();
}

} // namespace

class MemberStoreTest : public QObject {
    Q_OBJECT
private slots:
    void createsLoginReadyAccount() {
        Fixture f;
        QVERIFY2(f.Open(), qPrintable(f.error));
        const auto credentials = f.store.Add("  Alice  ", f.error);
        QVERIFY2(credentials.has_value(), qPrintable(f.error));
        QCOMPARE(credentials->username, "Alice");
        QCOMPARE(credentials->email, "Alice@shadps4.local");
        QCOMPARE(credentials->password.size(), 16);
        QVERIFY(!credentials->token.isEmpty());

        // A separate connection exercises the server's credential check.
        const auto user = f.database.CheckUser("Alice", credentials->password, {}, false);
        QVERIFY(user.has_value());
        QVERIFY(!user->banned && !user->admin && !user->statAgent);
        QCOMPARE(user->hash.size(), 32);
        QCOMPARE(user->salt.size(), 64);
        QVERIFY(user->hash != credentials->password.toUtf8());
        QCOMPARE(user->emailCheck, "alice@shadps4.local");
        QVERIFY(f.database.GetAccountCreationTime(user->userId).value_or(0) > 0);
        QVERIFY(f.database.CheckUser("Alice", credentials->password, credentials->token, true));
        QVERIFY(!f.database.CheckUser("Alice", "wrong-password", {}, false));
        QVERIFY(!f.database.CheckUser("alice", credentials->password, {}, false));
        QVERIFY(!f.database.CheckUser("Alice", credentials->password, "wrong-token", true));

        QCOMPARE(f.store.List(f.error).size(), 1);
        QVERIFY(f.error.isEmpty());
        QVERIFY(!f.store.NeedsToken());
        QFile config(f.directory.filePath("shadnet.cfg"));
        QVERIFY(config.open(QIODevice::WriteOnly));
        config.write("EmailValidated=true\n");
        config.close();
        QVERIFY(f.store.NeedsToken());
    }

    void rejectsInvalidOrDuplicateMembers() {
        Fixture f;
        QVERIFY(f.Open());
        const auto original = f.store.Add("Alice", f.error);
        QVERIFY(original);
        for (const QString& name :
             {QString(), QString("ab"), QString("seventeenlettersx"), QString("has space"),
              QString("bad@name"), QString("DeletedUser"), QString("alice")}) {
            QVERIFY(!f.store.Add(name, f.error));
            QVERIFY(!f.error.isEmpty());
        }
        QVERIFY(!f.database.CreateAccount("Other", "password", "", "bob@shadps4.local"));
        QVERIFY(!f.store.Add("Bob", f.error));
        QVERIFY(f.error.contains("email"));
        QCOMPARE(f.store.List(f.error).size(), 2);
        QVERIFY(f.database.CheckUser("Alice", original->password, {}, false));
    }

    void rejectsMissingAndUnrelatedFiles() {
        Fixture f;
        QVERIFY(f.Open());
        const QString missing = f.directory.filePath("missing.db");
        QVERIFY(!f.store.Open(missing, f.error));
        QVERIFY(!QFile::exists(missing));

        const QString unrelated = f.directory.filePath("unrelated.db");
        const QString connectionName = f.database.Conn().connectionName() + "-unrelated";
        {
            auto other = QSqlDatabase::addDatabase("QSQLITE", connectionName);
            other.setDatabaseName(unrelated);
            QVERIFY(other.open());
            {
                QSqlQuery query(other);
                QVERIFY(query.exec("CREATE TABLE unrelated (value TEXT)"));
            }
            QVERIFY(!f.store.Open(unrelated, f.error));
            QVERIFY(!f.error.isEmpty());
            QCOMPARE(other.tables(), QStringList{"unrelated"});
            other.close();
        }
        QSqlDatabase::removeDatabase(connectionName);
        QCOMPARE(f.store.Path(), QFileInfo(f.Path()).canonicalFilePath());
        QVERIFY(f.store.Add("StillWorks", f.error));
    }

    void removesOnlySelectedAccountAndItsData() {
        Fixture f;
        QVERIFY(f.Open());
        const auto alice = f.store.Add("Alice", f.error);
        const auto bob = f.store.Add("Bob", f.error);
        QVERIFY(alice && bob);
        const auto aliceId = f.database.GetUserId("Alice").value();
        const auto bobId = f.database.GetUserId("Bob").value();
        int dataId = 42;
        QVERIFY(QDir().mkpath(f.directory.filePath("score_data")));
        for (const qlonglong id : {aliceId, bobId}) {
            QVERIFY(Exec(f.database, "INSERT INTO score VALUES ('TEST',1,?,0,100,NULL,NULL,?,1)",
                         {id, dataId}));
            QVERIFY(f.database.RecordUserTrophy(id, "TEST", 1, 1));
            QVERIFY(
                Exec(f.database, "INSERT INTO tus_variable VALUES ('TEST',?,1,7,1,?)", {id, id}));
            QVERIFY(Exec(f.database, "INSERT INTO tus_data VALUES ('TEST',?,1,X'01',X'02',1,1,?)",
                         {id, id}));
            QFile blob(f.directory.filePath(
                QString("score_data/%1.sdt").arg(dataId++, 20, 10, QChar('0'))));
            QVERIFY(blob.open(QIODevice::WriteOnly));
            blob.write("score data");
        }
        QVERIFY(f.database.SetRelStatus(aliceId, bobId, 1, 1));
        QVERIFY(Exec(f.database, "INSERT INTO tus_vuser_variable VALUES ('TEST','virtual',1,7,1,?)",
                     {static_cast<qlonglong>(aliceId)}));

        QString warning;
        QVERIFY2(f.store.Remove(aliceId, f.error, warning), qPrintable(f.error));
        QVERIFY(warning.isEmpty());
        QVERIFY(!f.database.CheckUser("Alice", alice->password, {}, false));
        QVERIFY(f.database.CheckUser("Bob", bob->password, {}, false));
        for (const QString& table : {"account", "account_timestamp", "score", "user_trophies",
                                     "tus_variable", "tus_data", "tus_vuser_variable"})
            QCOMPARE(Count(f.database, table), 1);
        QCOMPARE(Count(f.database, "friendship"), 0);
        QVERIFY(!QFile::exists(f.directory.filePath("score_data/00000000000000000042.sdt")));
        QVERIFY(QFile::exists(f.directory.filePath("score_data/00000000000000000043.sdt")));
        QVERIFY(!f.store.Remove(aliceId, f.error, warning));
    }

    void rollsBackFailedRemoval() {
        Fixture f;
        QVERIFY(f.Open());
        const auto credentials = f.store.Add("Alice", f.error);
        QVERIFY(credentials);
        const auto id = f.database.GetUserId("Alice").value();
        QVERIFY(f.database.RecordUserTrophy(id, "TEST", 1, 1));
        QVERIFY(Exec(f.database, "CREATE TRIGGER prevent_delete BEFORE DELETE ON account "
                                 "BEGIN SELECT RAISE(ABORT, 'test failure'); END"));
        QString warning;
        QVERIFY(!f.store.Remove(id, f.error, warning));
        QVERIFY(!f.error.isEmpty());
        QVERIFY(f.database.CheckUser("Alice", credentials->password, {}, false));
        QCOMPARE(Count(f.database, "user_trophies"), 1);
        QCOMPARE(Count(f.database, "account_timestamp"), 1);
    }
};

QTEST_GUILESS_MAIN(MemberStoreTest)
#include "member_store_test.moc"
