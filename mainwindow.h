#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCalendarWidget>
#include <QListWidget>
#include <QSystemTrayIcon>
#include <QSoundEffect>
#include <QMap>
#include <QTimer>
#include <QSqlDatabase>
#include <QActionGroup>
#include <QAction>

#include "ClientHttp.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void showLoginDialog();

    // UI
    void onDateChanged();
    void onHourDoubleClicked(QListWidgetItem* item);

    // Theme
    void changeTheme(QAction* action);

    // ClientHttp slots
    void onLoginResult(bool success, const QString &role, const QString &error = QString());
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
    QTimer* m_notesUpdateTimer;
    QString m_currentUserRole; // Поле для роли

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
};

#endif // MAINWINDOW_H
