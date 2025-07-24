#ifndef NOTESDATABASE_H
#define NOTESDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QDate>
#include <QString>
#include <QMap>
<<<<<<< HEAD
#include <QList>
=======
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b

class NotesDatabase : public QObject
{
    Q_OBJECT
public:
    explicit NotesDatabase(QObject *parent = nullptr);
    ~NotesDatabase();

    bool openDatabase(const QString& path);
    void closeDatabase();

<<<<<<< HEAD
    QMap<int, QMap<int, QString>> getNotesForDate(const QDate& date);
    bool saveNote(const QDate& date, int hour, int minute, const QString& text);
    bool deleteNote(const QDate& date, int hour, int minute);
=======
    QMap<int, QString> getNotesForDate(const QDate& date);
    bool saveNote(const QDate& date, int hour, const QString& text);
    bool deleteNote(const QDate& date, int hour);
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b
    QList<QDate> getDatesWithNotes();

private:
    QSqlDatabase db;
    bool createTableIfNotExists();
};

#endif // NOTESDATABASE_H
