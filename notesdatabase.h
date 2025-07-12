#ifndef NOTESDATABASE_H
#define NOTESDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QDate>
#include <QString>
#include <QMap>

class NotesDatabase : public QObject
{
    Q_OBJECT
public:
    explicit NotesDatabase(QObject *parent = nullptr);
    ~NotesDatabase();

    bool openDatabase(const QString& path);
    void closeDatabase();

    QMap<int, QString> getNotesForDate(const QDate& date);
    bool saveNote(const QDate& date, int hour, const QString& text);
    bool deleteNote(const QDate& date, int hour);
    QList<QDate> getDatesWithNotes();

private:
    QSqlDatabase db;
    bool createTableIfNotExists();
};

#endif // NOTESDATABASE_H
