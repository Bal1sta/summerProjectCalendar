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
<<<<<<< HEAD
                              "minute INTEGER,"
                              "note TEXT,"
                              "PRIMARY KEY(date, hour, minute))");
=======
                              "note TEXT,"
                              "PRIMARY KEY(date, hour))");
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b
    if (!success) {
        qWarning() << "Failed to create table:" << query.lastError().text();
    }
    return success;
}

<<<<<<< HEAD
QMap<int, QMap<int, QString>> NotesDatabase::getNotesForDate(const QDate& date)
{
    QMap<int, QMap<int, QString>> notes;
    QSqlQuery query(db);
    query.prepare("SELECT hour, minute, note FROM notes WHERE date = ?");
=======
QMap<int, QString> NotesDatabase::getNotesForDate(const QDate& date)
{
    QMap<int, QString> notes;
    QSqlQuery query(db);
    query.prepare("SELECT hour, note FROM notes WHERE date = ?");
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b
    query.addBindValue(date.toString(Qt::ISODate));
    if (query.exec()) {
        while (query.next()) {
            int hour = query.value(0).toInt();
<<<<<<< HEAD
            int minute = query.value(1).toInt();
            QString note = query.value(2).toString();
            notes[hour][minute] = note;
=======
            QString note = query.value(1).toString();
            notes[hour] = note;
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b
        }
    } else {
        qWarning() << "Failed to fetch notes:" << query.lastError().text();
    }
    return notes;
}

<<<<<<< HEAD
bool NotesDatabase::saveNote(const QDate& date, int hour, int minute, const QString& text)
{
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO notes (date, hour, minute, note) VALUES (?, ?, ?, ?)");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
    query.addBindValue(minute);
=======
bool NotesDatabase::saveNote(const QDate& date, int hour, const QString& text)
{
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO notes (date, hour, note) VALUES (?, ?, ?)");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b
    query.addBindValue(text);
    if (!query.exec()) {
        qWarning() << "Failed to save note:" << query.lastError().text();
        return false;
    }
    return true;
}

<<<<<<< HEAD
bool NotesDatabase::deleteNote(const QDate& date, int hour, int minute)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM notes WHERE date = ? AND hour = ? AND minute = ?");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
    query.addBindValue(minute);
=======
bool NotesDatabase::deleteNote(const QDate& date, int hour)
{
    QSqlQuery query(db);
    query.prepare("DELETE FROM notes WHERE date = ? AND hour = ?");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b
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
