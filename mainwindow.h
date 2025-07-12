#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCalendarWidget>
#include <QListWidget>
#include <QSqlDatabase>
#include <QDate>
#include <QStyledItemDelegate>
#include <QCloseEvent>
#include <QTimer>
#include <QMap>
#include <QSystemTrayIcon>
#include <QSoundEffect>



class ItemDelegate : public QStyledItemDelegate {
public:
    explicit ItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(size.height() + 10);
        return size;
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QCalendarWidget* calendar;
    QListWidget* list;
    QSqlDatabase db;

    QSystemTrayIcon* trayIcon;
    QMap<QString, QTimer*> notificationTimers; // ключ: "yyyy-MM-dd_hh"
    QSoundEffect notificationSound;

    void fillHoursList();
    void highlightCurrentDate();
    void highlightDatesWithNotes();
    void loadNotesForDate(const QDate& date);
    void saveNote(const QDate& date, int hour, const QString& text);
    void deleteNote(const QDate& date, int hour);

    void setupTrayIcon();
    void scheduleNotifications();
    void scheduleNotificationFor(const QDate& date, int hour, const QString& note);
    void clearAllNotificationTimers();

private slots:
    void onDateChanged();
    void onHourDoubleClicked(QListWidgetItem* item);
    void showNotification();

//*****************************************
// public slots:
//     void testNotification();
//*****************************************

};

#endif // MAINWINDOW_H
