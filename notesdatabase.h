#ifndef NOTESDATABASE_H
#define NOTESDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QDate>
#include <QString>
#include <QMap>
#include <QList>

class NotesDatabase : public QObject
{
    Q_OBJECT
public:
    explicit NotesDatabase(QObject *parent = nullptr);
    ~NotesDatabase();

    bool openDatabase(const QString& path);
    void closeDatabase();

    QMap<int, QMap<int, QString>> getNotesForDate(const QDate& date);
    bool saveNote(const QDate& date, int hour, int minute, const QString& text);
    bool deleteNote(const QDate& date, int hour, int minute);
    QList<QDate> getDatesWithNotes();

private:
    QSqlDatabase db;
    bool createTableIfNotExists();
};

#endif // NOTESDATABASE_H
