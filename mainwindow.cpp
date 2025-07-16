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
#include <QSettings>
#include <QMenuBar>
#include <QPalette>

// ---- Палитры для темы ----
void applyDarkPalette(QApplication &app) {
    app.setStyle("Fusion");
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53,53,53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25,25,25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53,53,53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::black);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53,53,53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Highlight, QColor(42,130,218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    app.setPalette(darkPalette);
}

void applyLightPalette(QApplication &app) {
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());
}

// -------------------------

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

    // --- Путь к БД ---
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
    if(!query.exec("CREATE TABLE IF NOT EXISTS notes ("
                    "date TEXT,"
                    "hour INTEGER,"
                    "note TEXT,"
                    "PRIMARY KEY(date, hour))")) {
        QMessageBox::critical(this, "Ошибка SQL", query.lastError().text());
        return;
    }

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

    QApplication::setWindowIcon(QIcon(":/icons/icons/calendar_32x32.png"));

    // Меню темы
    viewMenu = menuBar()->addMenu(tr("Вид"));

    themeActionGroup = new QActionGroup(this);
    lightThemeAction = new QAction(tr("Светлая"), this);
    lightThemeAction->setCheckable(true);

    darkThemeAction = new QAction(tr("Тёмная"), this);
    darkThemeAction->setCheckable(true);

    themeActionGroup->addAction(lightThemeAction);
    themeActionGroup->addAction(darkThemeAction);

    viewMenu->addAction(lightThemeAction);
    viewMenu->addAction(darkThemeAction);

    connect(themeActionGroup, &QActionGroup::triggered, this, &MainWindow::changeTheme);

    loadSavedTheme();
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

    if (m_isDarkTheme) {
        format.setBackground(QBrush(QColor(0, 122, 204))); // синий фон
        format.setForeground(QBrush(Qt::black));
    } else {
        format.setBackground(QBrush(QColor(0, 122, 204)));
        format.setForeground(QBrush(Qt::white));
    }

    QDate today = QDate::currentDate();
    calendar->setDateTextFormat(today, format);
}

void MainWindow::highlightDatesWithNotes()
{
    calendar->setDateTextFormat(QDate(), QTextCharFormat()); // сброс формата
    highlightCurrentDate();

    QTextCharFormat noteFormat;
    if (m_isDarkTheme) {
        noteFormat.setBackground(QBrush(QColor(Qt::darkGray)));
        noteFormat.setForeground(QBrush(Qt::black));
    } else {
        noteFormat.setBackground(QBrush(QColor(Qt::lightGray)));
        noteFormat.setForeground(QBrush(Qt::black));
    }

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
    if (!query.exec()){
        qDebug() << "Ошибка сохранения заметки:" << query.lastError().text();
    }
}

void MainWindow::deleteNote(const QDate& date, int hour)
{
    QSqlQuery query;
    query.prepare("DELETE FROM notes WHERE date = ? AND hour = ?");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
    if (!query.exec()){
        qDebug() << "Ошибка удаления заметки:" << query.lastError().text();
    }
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
}

void MainWindow::applyDarkTheme()
{
    m_isDarkTheme = true;

    applyDarkPalette(*qApp);

    calendar->setStyleSheet(R"(
        QCalendarWidget QWidget { background-color: #232629; color: #f0f0f0; }
        QCalendarWidget QAbstractItemView { background-color: #232629; color: #f0f0f0; selection-background-color: #3d4250; selection-color: #ffffff; }
        QCalendarWidget QToolButton { background-color: #232629; color: #f0f0f0; }
        QCalendarWidget QSpinBox { background-color: #232629; color: #f0f0f0; }
        QCalendarWidget QMenu { background-color: #232629; color: #f0f0f0; }
    )");

    list->setStyleSheet(R"(
        QListWidget { background-color: #232629; color: #f0f0f0; border: 1px solid #3d4250; }
        QListWidget::item { background-color: #232629; color: #f0f0f0; }
        QListWidget::item:selected { background-color: #3d4250; color: #ffffff; }
    )");

    highlightCurrentDate();
    highlightDatesWithNotes();
    fillHoursList();
}

void MainWindow::applyLightTheme()
{
    m_isDarkTheme = false;

    applyLightPalette(*qApp);

    calendar->setStyleSheet(R"(
        QCalendarWidget QWidget { background-color: #ffffff; color: #000000; }
        QCalendarWidget QAbstractItemView { background-color: #ffffff; color: #000000; selection-background-color: #cce8ff; selection-color: #000000; }
        QCalendarWidget QToolButton { background-color: #f0f0f0; color: #000000; }
        QCalendarWidget QSpinBox { background-color: #f0f0f0; color: #000000; }
        QCalendarWidget QMenu { background-color: #f0f0f0; color: #000000; }
    )");

    list->setStyleSheet(R"(
        QListWidget { background-color: #ffffff; color: #000000; border: 1px solid #cccccc; }
        QListWidget::item { background-color: #ffffff; color: #000000; }
        QListWidget::item:selected { background-color: #cce8ff; color: #000000; }
    )");

    highlightCurrentDate();
    highlightDatesWithNotes();
    fillHoursList();
}

void MainWindow::loadSavedTheme()
{
    QSettings settings("MyCompany", "MyApp");
    QString theme = settings.value("theme", "Dark").toString();

    if (theme == "Light") {
        lightThemeAction->setChecked(true);
        applyLightTheme();
    } else {
        darkThemeAction->setChecked(true);
        applyDarkTheme();
    }
}

void MainWindow::saveTheme(const QString& themeName)
{
    QSettings settings("MyCompany", "MyApp");
    settings.setValue("theme", themeName);
}

void MainWindow::changeTheme(QAction* action)
{
    if (!action)
        return;

    if (action == lightThemeAction) {
        applyLightTheme();
        saveTheme("Light");
    } else if (action == darkThemeAction) {
        applyDarkTheme();
        saveTheme("Dark");
    }
}
