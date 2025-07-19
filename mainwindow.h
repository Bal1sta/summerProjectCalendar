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
#include <QActionGroup>
#include <QMenu>
#include "client.h"

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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QCalendarWidget* calendar;
    QListWidget* list;
    QSqlDatabase db;

    Client* client;

    QSystemTrayIcon* trayIcon;
    QMap<QString, QTimer*> notificationTimers;
    QSoundEffect notificationSound;

    QMenu *viewMenu;
    QActionGroup* themeActionGroup;
    QAction* lightThemeAction;
    QAction* darkThemeAction;

    bool m_isDarkTheme = true;
    QMap<int, QJsonObject> networkEventsCache;

    void fillHoursList();
    void highlightCurrentDate();
    void highlightDatesWithNotes();
    void loadNotesForDate(const QDate& date);
    void saveNote(const QDate& date, int hour, int minute, const QString& text);
    void deleteNote(const QDate& date, int hour, int minute);

    void setupTrayIcon();
    void scheduleNotifications();
    void scheduleNotificationFor(const QDate& date, int hour, int minute, const QString& note);
    void clearAllNotificationTimers();

    void setupThemeMenu();
    void applyLightTheme();
    void applyDarkTheme();
    void loadSavedTheme();
    void saveTheme(const QString& themeName);

    void processNetworkEvent(const QJsonObject& event);
    void removeNetworkEvent(int id);

private slots:
    void onDateChanged();
    void onHourDoubleClicked(QListWidgetItem* item);
    void showNotification();
    void changeTheme(QAction* action);
    void showLoginDialog();
};

#endif // MAINWINDOW_H
