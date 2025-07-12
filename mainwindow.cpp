#include "mainwindow.h"
#include <QHBoxLayout>
#include <QInputDialog>
#include <QTextCharFormat>
#include <QBrush>
#include <QColor>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>
#include <QUrl>
#include <QDir>
#include <QApplication>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #808080;");
    setCentralWidget(centralWidget);

    calendar = new QCalendarWidget(this);
    calendar->setStyleSheet(R"(
    QCalendarWidget QWidget { background-color: #232629; color: #f0f0f0; }
    QCalendarWidget QAbstractItemView { background-color: #232629; color: #f0f0f0; selection-background-color: #3d4250; selection-color: #ffffff; }
    QCalendarWidget QToolButton { background-color: #232629; color: #f0f0f0; }
    QCalendarWidget QSpinBox { background-color: #232629; color: #f0f0f0; }
    QCalendarWidget QMenu { background-color: #232629; color: #f0f0f0; }
    )");

    list = new QListWidget(this);
    list->setStyleSheet(R"(
    QListWidget { background-color: #232629; color: #f0f0f0; border: 1px solid #3d4250; }
    QListWidget::item { background-color: #232629; color: #f0f0f0; }
    QListWidget::item:selected { background-color: #3d4250; color: #ffffff; }
    )");

    QHBoxLayout* layout = new QHBoxLayout(centralWidget);
    layout->addWidget(calendar);
    layout->addWidget(list);

    // --- Указываем полный путь к базе данных ---
    QString folderPath = R"(C:\_LABY_\4-Semestr\Proga_2\summerProject\summerProject)";
    QDir dir(folderPath);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            QMessageBox::critical(this, "Ошибка", "Не удалось создать папку для базы данных");
            return;
        }
    }

    db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = folderPath + "/notes.db";
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        QMessageBox::critical(this, "Ошибка базы данных", db.lastError().text());
        return;
    }

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS notes ("
               "date TEXT,"
               "hour INTEGER,"
               "note TEXT,"
               "PRIMARY KEY(date, hour))");

    fillHoursList();
    highlightCurrentDate();
    highlightDatesWithNotes();

    connect(calendar, &QCalendarWidget::selectionChanged, this, &MainWindow::onDateChanged);
    connect(list, &QListWidget::itemDoubleClicked, this, &MainWindow::onHourDoubleClicked);

    setMinimumSize(800, 600);
    list->setItemDelegate(new ItemDelegate(list));

    setupTrayIcon();

    notificationSound.setSource(QUrl("qrc:/sounds/sounds/archivo.wav"));
    notificationSound.setVolume(0.5f);

    scheduleNotifications();

    // //*************************************
    // setupTrayIcon();

    // notificationSound.setSource(QUrl("qrc:/sounds/sounds/archivo.wav"));
    // notificationSound.setVolume(0.5f);

    // scheduleNotifications();

    // // Запускаем тестовое уведомление через 1 секунду после старта
    // QTimer::singleShot(1000, this, &MainWindow::testNotification);
    // //*************************************

    QApplication::setWindowIcon(QIcon(":/icons/icons/calendar_32x32.png"));
}

MainWindow::~MainWindow()
{
    clearAllNotificationTimers();
}

void MainWindow::fillHoursList()
{
    list->clear();
    for (int hour = 0; hour < 24; ++hour) {
        list->addItem(QString("%1:00").arg(hour, 2, 10, QChar('0')));
    }
    loadNotesForDate(calendar->selectedDate());
}

void MainWindow::highlightCurrentDate()
{
    QTextCharFormat format;
    format.setBackground(QBrush(QColor(0, 122, 204))); // синий фон
    format.setForeground(QBrush(Qt::black));
    QDate today = QDate::currentDate();
    calendar->setDateTextFormat(today, format);
}

void MainWindow::highlightDatesWithNotes()
{
    calendar->setDateTextFormat(QDate(), QTextCharFormat()); // сброс формата
    highlightCurrentDate();

    QTextCharFormat noteFormat;
    noteFormat.setBackground(QBrush(QColor(Qt::darkGray)));
    noteFormat.setForeground(QBrush(Qt::black));

    QSqlQuery query("SELECT DISTINCT date FROM notes");
    while (query.next()) {
        QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
        calendar->setDateTextFormat(date, noteFormat);
    }
}

void MainWindow::loadNotesForDate(const QDate& date)
{
    QSqlQuery query;
    query.prepare("SELECT hour, note FROM notes WHERE date = ?");
    query.addBindValue(date.toString(Qt::ISODate));
    query.exec();

    while (query.next()) {
        int hour = query.value(0).toInt();
        QString note = query.value(1).toString();
        if (hour >= 0 && hour < list->count()) {
            list->item(hour)->setText(QString("%1:00 - %2").arg(hour, 2, 10, QChar('0')).arg(note));
        }
    }
}

void MainWindow::saveNote(const QDate& date, int hour, const QString& text)
{
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO notes (date, hour, note) VALUES (?, ?, ?)");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
    query.addBindValue(text);
    query.exec();
}

void MainWindow::deleteNote(const QDate& date, int hour)
{
    QSqlQuery query;
    query.prepare("DELETE FROM notes WHERE date = ? AND hour = ?");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
    query.exec();
}

void MainWindow::onDateChanged()
{
    fillHoursList();
}

void MainWindow::onHourDoubleClicked(QListWidgetItem* item)
{
    int hour = list->row(item);
    QDate selectedDate = calendar->selectedDate();

    QString currentNote;
    QSqlQuery query;
    query.prepare("SELECT note FROM notes WHERE date = ? AND hour = ?");
    query.addBindValue(selectedDate.toString(Qt::ISODate));
    query.addBindValue(hour);
    query.exec();
    if (query.next()) {
        currentNote = query.value(0).toString();
    }

    bool ok;
    QString text = QInputDialog::getText(this,
                                         QString("Заметка на %1 %2:00").arg(selectedDate.toString()).arg(hour),
                                         "Введите заметку:",
                                         QLineEdit::Normal,
                                         currentNote,
                                         &ok);
    if (ok) {
        if (text.isEmpty()) {
            deleteNote(selectedDate, hour);
            list->item(hour)->setText(QString("%1:00").arg(hour, 2, 10, QChar('0')));
        } else {
            saveNote(selectedDate, hour, text);
            list->item(hour)->setText(QString("%1:00 - %2").arg(hour, 2, 10, QChar('0')).arg(text));
        }
        highlightDatesWithNotes();

        scheduleNotifications();
    }
}

void MainWindow::setupTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icons/icons/calendar_32x32.png"));
    trayIcon->show();
}

void MainWindow::scheduleNotifications()
{
    clearAllNotificationTimers();

    QSqlQuery query("SELECT date, hour, note FROM notes");
    while (query.next()) {
        QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
        int hour = query.value(1).toInt();
        QString note = query.value(2).toString();

        scheduleNotificationFor(date, hour, note);
    }
}

void MainWindow::scheduleNotificationFor(const QDate& date, int hour, const QString& note)
{
    QDateTime noteDateTime(date, QTime(hour, 0, 0));
    QDateTime notifyTime = noteDateTime.addSecs(-30 * 60); // за 30 минут

    QDateTime now = QDateTime::currentDateTime();

    if (notifyTime <= now) {
        return; // Время уведомления уже прошло
    }

    qint64 msecToNotify = now.msecsTo(notifyTime);
    if (msecToNotify <= 0)
        return;

    QString key = date.toString(Qt::ISODate) + "_" + QString::number(hour);

    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &MainWindow::showNotification);

    notificationTimers[key] = timer;

    timer->setProperty("noteText", note);
    timer->setProperty("noteDateTime", noteDateTime);

    timer->start(msecToNotify);

    qDebug() << "Scheduled notification for" << notifyTime.toString() << "note:" << note;
}

void MainWindow::clearAllNotificationTimers()
{
    for (auto timer : notificationTimers) {
        timer->stop();
        timer->deleteLater();
    }
    notificationTimers.clear();
}

void MainWindow::showNotification()
{
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if (!timer)
        return;

    QString noteText = timer->property("noteText").toString();
    QDateTime noteDateTime = timer->property("noteDateTime").toDateTime();

    QString title = QString("Напоминание на %1").arg(noteDateTime.toString("dd.MM.yyyy HH:mm"));
    QString message = noteText;

    trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 10000);

    if (notificationSound.isLoaded())
        notificationSound.play();
}

// //*****************************************
// void MainWindow::testNotification()
// {
//     QString title = "Тестовое уведомление";
//     QString message = "Это проверка уведомления со звуком.";

//     trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 10000);

//     if (notificationSound.isLoaded())
//         notificationSound.play();
// }
// //*****************************************

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
}
