#include "mainwindow.h"
#include "logindialog.h"

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
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QJsonObject>
#include <QJsonDocument>

// ---- Палитры темы --------------------------------------------------------------------
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

// ---- Реализация MainWindow -------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), client(new Client(this))
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

    // Путь к базе данных — измените на удобный для себя путь
    QString folderPath = QDir::homePath() + "/summerProjectData";
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
    // Добавляем id для связи с сервером (УНИКАЛЬНЫЙ ПРИМАРИ КЕЙ)
    if (!query.exec("CREATE TABLE IF NOT EXISTS notes ("
                    "id INTEGER PRIMARY KEY UNIQUE,"
                    "date TEXT,"
                    "hour INTEGER,"
                    "minute INTEGER,"
                    "note TEXT"
                    ")")) {
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

    notificationSound.setSource(QUrl("qrc:/sounds/archivo.wav"));
    notificationSound.setVolume(0.5f);

    scheduleNotifications();

    QApplication::setWindowIcon(QIcon(":/icons/calendar_32x32.png"));

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

    // Подключение клиента и обработка сетевых событий
    connect(client, &Client::loginResult, this, [this](bool success){
        if (success) {
            QMessageBox::information(this, "Успех", "Авторизация прошла успешно");
            fillHoursList();
            highlightDatesWithNotes();
            scheduleNotifications();
        } else {
            QMessageBox::critical(this, "Ошибка", "Неверный логин или пароль");
            showLoginDialog();
        }
    });
    // connect(client, &Client::eventUpdated, this, &MainWindow::processNetworkEvent);
    // connect(client, &Client::eventDeleted, this, &MainWindow::removeNetworkEvent);

    client->connectToServer("127.0.0.1", 12345);

    showLoginDialog();
}

MainWindow::~MainWindow()
{
    clearAllNotificationTimers();
}

void MainWindow::showLoginDialog()
{
    LoginDialog *dlg = new LoginDialog(this);
    dlg->setModal(true);

    connect(dlg, &LoginDialog::loginRequested, this, [this, dlg](const QString &user, const QString &pass){
        if (client->isConnected()) {
            client->login(user, pass);
        } else {
            QMessageBox::warning(this, "Ошибка", "Нет соединения с сервером");
        }
    });

    connect(client, &Client::loginResult, this, [this, dlg](bool success){
        if (success) {
            QMessageBox::information(this, "Успех", "Авторизация прошла успешно");
            dlg->accept(); // Закрываем диалог входа
        } else {
            QMessageBox::warning(dlg, "Ошибка", "Неверный логин или пароль");
            // Диалог остаётся открытым для повторного ввода
        }
    });

    // Если пользователь закроет окно или отменит - выходим из приложения
    if(dlg->exec() != QDialog::Accepted) {
        close();
    }

    dlg->deleteLater();
}


// Заполнение списка часов ---------------------------------------------------------------
void MainWindow::fillHoursList()
{
    list->clear();
    for (int hour=0; hour<24; ++hour)
        list->addItem(QString("%1:00").arg(hour,2,10,QChar('0')));

    loadNotesForDate(calendar->selectedDate());
}

// Подсвечиваем текущий день ---------------------------------------------------------------
void MainWindow::highlightCurrentDate()
{
    QTextCharFormat format;
    if(m_isDarkTheme) {
        format.setBackground(QBrush(QColor(0,122,204)));
        format.setForeground(QBrush(Qt::black));
    } else {
        format.setBackground(QBrush(QColor(0,122,204)));
        format.setForeground(QBrush(Qt::white));
    }
    QDate today = QDate::currentDate();
    calendar->setDateTextFormat(today, format);
}

// Подсвечиваем даты с заметками -----------------------------------------------------------
void MainWindow::highlightDatesWithNotes()
{
    calendar->setDateTextFormat(QDate(), QTextCharFormat());
    highlightCurrentDate();

    QTextCharFormat noteFormat;
    if(m_isDarkTheme){
        noteFormat.setBackground(QBrush(QColor(Qt::darkGray)));
        noteFormat.setForeground(QBrush(Qt::black));
    } else {
        noteFormat.setBackground(QBrush(QColor(Qt::lightGray)));
        noteFormat.setForeground(QBrush(Qt::black));
    }

    QSqlQuery query("SELECT DISTINCT date FROM notes");
    while(query.next()){
        QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
        calendar->setDateTextFormat(date, noteFormat);
    }
}

// Загружаем заметки на выбранную дату -----------------------------------------------------
void MainWindow::loadNotesForDate(const QDate& date)
{
    // Сброс текста в списке по часам
    for(int hour = 0; hour < list->count(); ++hour)
        list->item(hour)->setText(QString("%1:00").arg(hour, 2, 10, QChar('0')));

    QMap<int,QList<QPair<int,QString>>> notesByHour;
    QSqlQuery query;
    query.prepare("SELECT hour, minute, note FROM notes WHERE date = ?");
    query.addBindValue(date.toString(Qt::ISODate));
    if(!query.exec())
    {
        qDebug() << "Ошибка exec loadNotesForDate:" << query.lastError().text();
        return;
    }

    while(query.next()){
        int hour = query.value(0).toInt();
        int minute = query.value(1).toInt();
        QString note = query.value(2).toString();
        notesByHour[hour].append(qMakePair(minute,note));
    }

    for(auto it = notesByHour.constBegin(); it != notesByHour.constEnd(); ++it){
        int hour = it.key();
        const QList<QPair<int,QString>>& noteList = it.value();

        QStringList noteStrings;
        for(const auto& p : noteList){
            noteStrings.append(QString("%1:%2 - %3")
                                   .arg(hour,2,10,QChar('0'))
                                   .arg(p.first,2,10,QChar('0'))
                                   .arg(p.second));
        }
        QString combinedText = noteStrings.join(" | ");

        if(hour >=0 && hour < list->count()){
            list->item(hour)->setText(combinedText);
        }
    }
}

// Сохраняем заметку в базу и на сервер -----------------------------------------------
void MainWindow::saveNote(const QDate& date, int hour, int minute, const QString& text)
{
    // Локальное сохранение
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO notes (date, hour, minute, note) VALUES (?, ?, ?, ?)");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
    query.addBindValue(minute);
    query.addBindValue(text);

    if (!query.exec()) {
        qDebug() << "DB error:" << query.lastError().text();
    }

    // Отправка на сервер
    QJsonObject message;
    message["type"] = "update_note";
    message["date"] = date.toString(Qt::ISODate);
    message["hour"] = hour;
    message["minute"] = minute;
    message["note"] = text;

    client->sendMessage(message);
}

// Удаляем заметку из базы и с сервера -----------------------------------------------
void MainWindow::deleteNote(const QDate& date, int hour, int minute)
{
    QSqlQuery qCheck(db);
    qCheck.prepare("SELECT id FROM notes WHERE date = ? AND hour = ? AND minute = ?");
    qCheck.addBindValue(date.toString(Qt::ISODate));
    qCheck.addBindValue(hour);
    qCheck.addBindValue(minute);
    if (!qCheck.exec()) {
        qWarning() << "Ошибка проверки события:" << qCheck.lastError().text();
    }

    if (qCheck.next()){
        int id = qCheck.value(0).toInt();
        client->deleteEvent(id);
    }

    // Локальное удаление
    QSqlQuery query(db);
    query.prepare("DELETE FROM notes WHERE date = ? AND hour = ? AND minute = ?");
    query.addBindValue(date.toString(Qt::ISODate));
    query.addBindValue(hour);
    query.addBindValue(minute);
    if(!query.exec()){
        qDebug() << "Ошибка exec deleteNote:" << query.lastError().text();
    }
}

// Обработка смены даты ----------------------------------------------------------------
void MainWindow::onDateChanged()
{
    fillHoursList();
}

// Обработка редактирования заметки по двойному клику ----------------------------------
void MainWindow::onHourDoubleClicked(QListWidgetItem* item)
{
    int hour = list->row(item);
    QDate selectedDate = calendar->selectedDate();

    bool okMinute = false;
    int minute = QInputDialog::getInt(this,
                                      QString("Выбор минуты"),
                                      QString("Выберите минуту для %1:").arg(hour, 2, 10, QChar('0')),
                                      0, 0, 59, 1, &okMinute);
    if(!okMinute)
        return;

    QString currentNote;
    QSqlQuery query;
    query.prepare("SELECT note FROM notes WHERE date = ? AND hour = ? AND minute = ?");
    query.addBindValue(selectedDate.toString(Qt::ISODate));
    query.addBindValue(hour);
    query.addBindValue(minute);
    if(!query.exec()){
        qDebug() << "Ошибка exec чтения заметки:" << query.lastError().text();
        return;
    }
    if(query.next())
        currentNote = query.value(0).toString();

    bool okText;
    QString text = QInputDialog::getText(this,
                                         QString("Заметка на %1 %2:%3").arg(selectedDate.toString()).arg(hour,2,10,QChar('0')).arg(minute,2,10,QChar('0')),
                                         "Введите заметку:",
                                         QLineEdit::Normal,
                                         currentNote,
                                         &okText);

    if(okText){
        if(text.trimmed().isEmpty()){
            deleteNote(selectedDate, hour, minute);
        } else {
            saveNote(selectedDate, hour, minute, text.trimmed());
        }
        fillHoursList();
        highlightDatesWithNotes();
        scheduleNotifications();
    }
}

// Настройка иконки в трее --------------------------------------------------------------
void MainWindow::setupTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/icons/calendar_32x32.png"));
    trayIcon->show();
}

// Планирование уведомлений -------------------------------------------------------------
void MainWindow::scheduleNotifications()
{
    clearAllNotificationTimers();

    QSqlQuery query("SELECT date, hour, minute, note FROM notes");
    while(query.next()){
        QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
        int hour = query.value(1).toInt();
        int minute = query.value(2).toInt();
        QString note = query.value(3).toString();
        scheduleNotificationFor(date, hour, minute, note);
    }
}

void MainWindow::scheduleNotificationFor(const QDate& date, int hour, int minute, const QString& note)
{
    QDateTime noteDateTime(date, QTime(hour, minute, 0));
    QDateTime notifyTime = noteDateTime.addSecs(-30 * 60); // за 30 минут

    QDateTime now = QDateTime::currentDateTime();

    if(notifyTime <= now)
        return;

    qint64 msecToNotify = now.msecsTo(notifyTime);
    if(msecToNotify <= 0)
        return;

    QString key = date.toString(Qt::ISODate) + "_" + QString::number(hour) + "_" + QString::number(minute);

    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, this, &MainWindow::showNotification);

    notificationTimers[key] = timer;

    timer->setProperty("noteText", note);
    timer->setProperty("noteDateTime", noteDateTime);

    timer->start(msecToNotify);

    qDebug() << "Планируется уведомление на" << notifyTime.toString() << ", заметка:" << note;
}

void MainWindow::clearAllNotificationTimers()
{
    for(auto timer : notificationTimers) {
        timer->stop();
        timer->deleteLater();
    }
    notificationTimers.clear();
}

// Показать уведомление из таймера ----------------------------------------------------
void MainWindow::showNotification()
{
    QTimer* timer = qobject_cast<QTimer*>(sender());
    if(!timer)
        return;

    QString noteText = timer->property("noteText").toString();
    QDateTime noteDateTime = timer->property("noteDateTime").toDateTime();

    QString title = QString("Напоминание на %1").arg(noteDateTime.toString("dd.MM.yyyy HH:mm"));
    QString message = noteText;

    trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 10000);

    if(notificationSound.isLoaded())
        notificationSound.play();
}

// Обработка закрытия окна -----------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    event->accept();
}

// Применение тёмной темы --------------------------------------------------------------
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

// Применение светлой темы --------------------------------------------------------------
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

// Загрузка сохранённой темы из настроек -------------------------------------------
void MainWindow::loadSavedTheme()
{
    QSettings settings("MyCompany", "MyApp");
    QString theme = settings.value("theme", "Dark").toString();

    if(theme == "Light"){
        lightThemeAction->setChecked(true);
        applyLightTheme();
    } else {
        darkThemeAction->setChecked(true);
        applyDarkTheme();
    }
}

// Сохранение выбранной темы в настройки ---------------------------------------------
void MainWindow::saveTheme(const QString& themeName)
{
    QSettings settings("MyCompany", "MyApp");
    settings.setValue("theme", themeName);
}

// Обработка выбора темы в меню -------------------------------------------------------
void MainWindow::changeTheme(QAction* action)
{
    if (!action)
        return;

    if(action == lightThemeAction){
        applyLightTheme();
        saveTheme("Light");
    } else if(action == darkThemeAction){
        applyDarkTheme();
        saveTheme("Dark");
    }
}

// Обработка события обновления из сети ----------------------------------------------
void MainWindow::processNetworkEvent(const QJsonObject& event)
{
    int id = event["id"].toInt();
    QString dateStr = event["date"].toString();
    int hour = event["hour"].toInt();
    int minute = event["minute"].toInt();
    QString note = event["note"].toString();

    networkEventsCache[id] = event;

    // Сохранить или обновить в базе событий с id
    QSqlQuery query(db);
    query.prepare("INSERT OR REPLACE INTO notes (id, date, hour, minute, note) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(id);
    query.addBindValue(dateStr);
    query.addBindValue(hour);
    query.addBindValue(minute);
    query.addBindValue(note);
    if(!query.exec()){
        qWarning() << "Ошибка при сохранении события из сети:" << query.lastError().text();
    }

    if(QDate::fromString(dateStr, Qt::ISODate) == calendar->selectedDate())
        fillHoursList();

    highlightDatesWithNotes();
    scheduleNotifications();
}

// Обработка удаления события из сети -----------------------------------------------
void MainWindow::removeNetworkEvent(int id)
{
    if (!networkEventsCache.contains(id))
        return;

    QJsonObject event = networkEventsCache[id];
    QString dateStr = event["date"].toString();

    networkEventsCache.remove(id);

    QSqlQuery query(db);
    query.prepare("DELETE FROM notes WHERE id = ?");
    query.addBindValue(id);
    if(!query.exec()){
        qWarning() << "Ошибка при удалении события из сети:" << query.lastError().text();
    }

    if(QDate::fromString(dateStr, Qt::ISODate) == calendar->selectedDate())
        fillHoursList();

    highlightDatesWithNotes();
    scheduleNotifications();
}

//#include "mainwindow.moc"
