#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCalendarWidget>
#include <QListWidget>
<<<<<<< HEAD
#include <QSystemTrayIcon>
#include <QSoundEffect>
#include <QMap>
#include <QTimer>
#include <QSqlDatabase>
#include <QActionGroup>
#include <QAction>

#include "ClientHttp.h"
=======
#include <QSqlDatabase>
#include <QDate>
#include <QStyledItemDelegate>
#include <QCloseEvent>
#include <QTimer>
#include <QMap>
#include <QSystemTrayIcon>
#include <QSoundEffect>
#include <QActionGroup>
#include <QMenu>

class ItemDelegate : public QStyledItemDelegate {
public:
    explicit ItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(size.height() + 10);
        return size;
    }
};
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
<<<<<<< HEAD
=======

private:
    QCalendarWidget* calendar;
    QListWidget* list;
    QSqlDatabase db;

    QSystemTrayIcon* trayIcon;
    QMap<QString, QTimer*> notificationTimers; // ключ: "yyyy-MM-dd_hh"
    QSoundEffect notificationSound;

    // Тема и меню
    QMenu *viewMenu;
    QActionGroup* themeActionGroup;
    QAction* lightThemeAction;
    QAction* darkThemeAction;

    bool m_isDarkTheme = true;

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

    void setupThemeMenu();
    void applyLightTheme();
    void applyDarkTheme();
    void loadSavedTheme();
    void saveTheme(const QString& themeName);
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b

private slots:
    void showLoginDialog();

    // UI
    void onDateChanged();
    void onHourDoubleClicked(QListWidgetItem* item);
<<<<<<< HEAD

    // Theme
    void changeTheme(QAction* action);

    // ClientHttp slots
    void onLoginResult(bool success, const QString &error = QString());
    void onNotesReceived(const QJsonArray &notes);
    void onNoteAdded(const QJsonObject &note);
    void onNoteUpdated(const QJsonObject &note);
    void onNoteDeleted(int id);
    void onErrorOccurred(const QString &error);

    // Notifications
    void showNotification();

private:
    // UI elements
    QCalendarWidget* calendar;
    QListWidget* list;
    QSystemTrayIcon* trayIcon;

    // Local DB
    QSqlDatabase db;

    // Network client
    ClientHttp* client;

    // Cache notes by their ID
    QMap<int, QJsonObject> notesCache;

    // Notifications
    QMap<QString, QTimer*> notificationTimers;
    QSoundEffect notificationSound;

    // Themes
    QMenu* viewMenu;
    QActionGroup* themeActionGroup;
    QAction* lightThemeAction;
    QAction* darkThemeAction;
    bool m_isDarkTheme = true;

    // Helpers
    void fillHoursList();
    void highlightCurrentDate();
    void highlightDatesWithNotes();
    void loadNotesForDate(const QDate &date);

    void saveNote(const QDate &date, int hour, int minute, const QString &text);
    void deleteNoteById(int id);

    void setupTrayIcon();

    void scheduleNotifications();
    void scheduleNotificationFor(const QDate &date, int hour, int minute, const QString &note);
    void clearAllNotificationTimers();

    void applyDarkTheme();
    void applyLightTheme();
    void loadSavedTheme();
    void saveTheme(const QString &themeName);
=======
    void showNotification();
    void changeTheme(QAction* action);
>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b
};

#endif // MAINWINDOW_H
