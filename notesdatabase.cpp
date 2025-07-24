#include "notesdatabase.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

NotesDatabase::NotesDatabase(QObject *parent)
    : QObject(parent)
{
}

NotesDatabase::~NotesDatabase()
{
    closeDatabase();
}

bool NotesDatabase::openDatabase(const QString& path)
{
    if (QSqlDatabase::contains("notes_connection"))
        db = QSqlDatabase::database("notes_connection");
    else
        db = QSqlDatabase::addDatabase("QSQLITE", "notes_connection");

    db.setDatabaseName(path);
    if (!db.open()) {
        qWarning() << "Failed to open DB:" << db.lastError().text();
        return false;
    }
    return createTableIfNotExists();
}

void NotesDatabase::closeDatabase()
{
    if (db.isOpen()) {
        db.close();
    }
}

bool NotesDatabase::createTableIfNotExists()
{
    QSqlQuery query(db);
    bool success = query.exec("CREATE TABLE IF NOT EXISTS notes ("
                              "date TEXT,"
                              "hour INTEGER,"
                              "minute INTEGER,"
                              "note TEXT,"
                              "PRIMARY KEY(date, hour, minute))");
    if (!success) {
        qWarning() << "Failed to create table:" << query.lastError().text();
    }
    return success;
}

QMap<int, QMap<int, QString>> NotesDatabase::getNotesForDate(const QDate& date)
{
    QMap<int, QMap<int, QString>> notes;
    QSqlQuery query(db);
    query.prepare("SELECT hour, minute, note FROM notes WHERE date = ?");
    query.addBindValue(date.toString(Qt::ISODate));
    if (query.exec()) {
        while (query.next()) {
            int hour = query.value(0).toInt();
            int minute = query.value(1).toInt();
            QString note = query.value(2).toString();
            notes[hour][minute] = note;
        }
    } else {
        qWarning() << "Failed to fetch notes:" << query.lastError().text();
    }
    return notes;
}

bool NotesDatabase::saveNote(const QDate& date, int hour, int minute, const QString& text)
{
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO notes (date, hour, minute, note) VALUES (?, ?, ?, ?)");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
    query.addBindValue(minute);
    query.addBindValue(text);
    if (!query.exec()) {
        qWarning() << "Failed to save note:" << query.lastError().text();
        return false;
    }
    return true;
}

bool NotesDatabase::deleteNote(const QDate& date, int hour, int minute)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM notes WHERE date = ? AND hour = ? AND minute = ?");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
    query.addBindValue(minute);
    if (!query.exec()) {
        qWarning() << "Failed to delete note:" << query.lastError().text();
        return false;
    }
    return true;
}

QList<QDate> NotesDatabase::getDatesWithNotes()
{
    QList<QDate> dates;
    QSqlQuery query(db);
    if (query.exec("SELECT DISTINCT date FROM notes")) {
        while (query.next()) {
            QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
            dates.append(date);
        }
    } else {
        qWarning() << "Failed to fetch dates with notes:" << query.lastError().text();
    }
    return dates;
}
