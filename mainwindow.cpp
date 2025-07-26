#include "mainwindow.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDir>
#include <QSettings>
#include <QMenuBar>
#include <QCloseEvent>
#include <QPushButton>
#include <QJsonArray>
#include <QTextCharFormat>

// --- Темные и светлые палитры ---

static void applyDarkPalette(QApplication& app) {
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

static void applyLightPalette(QApplication& app) {
    app.setStyle("Fusion");
    app.setPalette(app.style()->standardPalette());
}

// --- Конструктор MainWindow ---

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    client(new ClientHttp(this))
{
    QWidget* centralWidget = new QWidget(this);
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

    auto* layout = new QHBoxLayout(centralWidget);
    layout->addWidget(calendar);
    layout->addWidget(list);

    QString folderPath = QDir::homePath() + "/summerProjectData";
    QDir dir(folderPath);
    if (!dir.exists() && !dir.mkpath(".")) {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать папку для базы данных");
        return;
    }

    db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = folderPath + "/notes.db";
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        QMessageBox::critical(this, "Ошибка БД", db.lastError().text());
        return;
    }

    QSqlQuery query;
    if (!query.exec("CREATE TABLE IF NOT EXISTS notes ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "date TEXT,"
                    "hour INTEGER,"
                    "minute INTEGER,"
                    "note TEXT)")) {
        QMessageBox::critical(this, "Ошибка SQL", query.lastError().text());
        return;
    }

    fillHoursList();
    highlightCurrentDate();
    highlightDatesWithNotes();

    connect(calendar, &QCalendarWidget::selectionChanged, this, &MainWindow::onDateChanged);
    connect(list, &QListWidget::itemDoubleClicked, this, &MainWindow::onHourDoubleClicked);

    setMinimumSize(800, 600);

    setupTrayIcon();

    notificationSound.setSource(QUrl("qrc:/sounds/sounds/archivo.wav"));
    notificationSound.setVolume(0.5f);

    QApplication::setWindowIcon(QIcon(":/icons/icons/calendar_32x32.png"));

    // Темы меню
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

    // Сигналы ClientHttp
    connect(client, &ClientHttp::loginResult, this, &MainWindow::onLoginResult);
    connect(client, &ClientHttp::notesReceived, this, &MainWindow::onNotesReceived);
    connect(client, &ClientHttp::noteAdded, this, &MainWindow::onNoteAdded);
    connect(client, &ClientHttp::noteUpdated, this, &MainWindow::onNoteUpdated);
    connect(client, &ClientHttp::noteDeleted, this, &MainWindow::onNoteDeleted);
    connect(client, &ClientHttp::errorOccurred, this, &MainWindow::onErrorOccurred);

    connect(client, &ClientHttp::registerResult, this, [&](bool success, const QString &msg){
        if(success) {
            QMessageBox::information(this, "Регистрация", msg);
        } else {
            QMessageBox::warning(this, "Ошибка регистрации", msg);
            showLoginDialog();
        }
    });

    // <-- ИЗМЕНЕНИЕ: Инициализация и подключение таймера для обновления заметок
    m_notesUpdateTimer = new QTimer(this);
    connect(m_notesUpdateTimer, &QTimer::timeout, this, [this](){
        // Запрашиваем обновления, только если окно видимо и клиент существует
        if (client && this->isVisible()) {
            client->fetchNotes();
        }
    });

    showLoginDialog();
}

MainWindow::~MainWindow()
{
    clearAllNotificationTimers();
    if(m_notesUpdateTimer) { // <-- ИЗМЕНЕНИЕ: Безопасная остановка таймера
        m_notesUpdateTimer->stop();
    }
}

void MainWindow::showLoginDialog()
{
    // Этот внешний цикл нужен, чтобы после неудачной попытки входа
    // или регистрации мы снова показывали этот главный диалог выбора.
    while (true) {
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("Вход / Регистрация");
        msgBox.setText("Для продолжения работы войдите в свою учетную запись или зарегистрируйтесь.");
        msgBox.setIcon(QMessageBox::Question);

        QPushButton *loginButton = msgBox.addButton(tr("Войти"), QMessageBox::AcceptRole);
        QPushButton *registerButton = msgBox.addButton(tr("Регистрация"), QMessageBox::ActionRole);
        QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);

        msgBox.exec();

        // Если пользователь нажал "Отмена" или закрыл окно
        if(msgBox.clickedButton() == cancelButton || msgBox.clickedButton() == nullptr) {
            close(); // Закрываем все приложение
            return;
        }

        // --- БЛОК ЗАПРОСА URL СЕРВЕРА (ЕСЛИ НЕОБХОДИМО) ---
        // Мы размещаем его здесь, так как он нужен и для входа, и для регистрации.
        if (client->serverUrl().isEmpty()) {
            QString url;
            while (true) {
                bool okUrl = false;
                url = QInputDialog::getText(this, "Адрес сервера",
                                            "Введите URL вашего сервера:", QLineEdit::Normal,
                                            "http://127.0.0.1:8000", &okUrl);
                if (!okUrl) { // Пользователь нажал "Отмена"
                    url.clear(); // Убедимся, что url пуст для проверки ниже
                    break;
                }
                if (url.trimmed().isEmpty()) {
                    QMessageBox::warning(this, "Ошибка ввода", "Адрес сервера не может быть пустым.");
                } else {
                    break; // URL введен корректно
                }
            }

            // Если пользователь отменил ввод URL, возвращаемся к главному выбору
            if (url.isEmpty()) {
                continue;
            }
            client->setServerUrl(url); // Устанавливаем URL в клиенте
        }

        if(msgBox.clickedButton() == registerButton) {
            QString username;
            // --- ЦИКЛ ВАЛИДАЦИИ ДЛЯ ИМЕНИ ПОЛЬЗОВАТЕЛЯ ---
            while(true) {
                bool okUser = false;
                username = QInputDialog::getText(this, "Регистрация", "Придумайте имя пользователя:", QLineEdit::Normal, "", &okUser);

                if (!okUser) break; // Пользователь нажал "Отмена", выходим из цикла валидации

                if (username.trimmed().isEmpty()) {
                    QMessageBox::warning(this, "Ошибка ввода", "Имя пользователя не может быть пустым. Пожалуйста, введите его.");
                    // Цикл while продолжится, и диалог появится снова
                } else {
                    break; // Ввод корректный, выходим из цикла валидации
                }
            }
            if (username.trimmed().isEmpty()) continue; // Если пользователь нажал "Отмена", возвращаемся к главному выбору

            QString password;
            // --- ЦИКЛ ВАЛИДАЦИИ ДЛЯ ПАРОЛЯ ---
            while(true) {
                bool okPass = false;
                password = QInputDialog::getText(this, "Регистрация", "Придумайте пароль:", QLineEdit::Password, "", &okPass);

                if (!okPass) break; // Пользователь нажал "Отмена"

                if (password.isEmpty()) {
                    QMessageBox::warning(this, "Ошибка ввода", "Пароль не может быть пустым. Пожалуйста, введите его.");
                } else {
                    break; // Ввод корректный
                }
            }
            if (password.isEmpty()) continue; // Если пользователь нажал "Отмена", возвращаемся к главному выбору

            // Если все данные введены, отправляем запрос
            client->registerUser(username, password);
            return; // Выходим из showLoginDialog, ждем асинхронного ответа
        }
        else if(msgBox.clickedButton() == loginButton) {
            QString user;
            while(true) {
                bool okUser = false;
                user = QInputDialog::getText(this, "Вход", "Введите имя пользователя:", QLineEdit::Normal, "", &okUser);
                if (!okUser) break;
                if (user.trimmed().isEmpty()) {
                    QMessageBox::warning(this, "Ошибка ввода", "Имя пользователя не может быть пустым.");
                } else {
                    break;
                }
            }
            if (user.trimmed().isEmpty()) continue;

            QString pass;
            while(true) {
                bool okPass = false;
                pass = QInputDialog::getText(this, "Вход", "Введите пароль:", QLineEdit::Password, "", &okPass);
                if (!okPass) break;
                if (pass.isEmpty()) {
                    QMessageBox::warning(this, "Ошибка ввода", "Пароль не может быть пустым.");
                } else {
                    break;
                }
            }
            if(pass.isEmpty()) continue;

            // Если все данные введены, отправляем запрос
            client->login(user, pass);
            return; // Выходим из showLoginDialog, ждем асинхронного ответа
        }
    }
}

void MainWindow::onLoginResult(bool success, const QString &role, const QString &error)
{
    if(success){
        QMessageBox::information(this, "Авторизация", "Успешный вход");
        m_currentUserRole = role; // Сохраняем роль

        if (m_currentUserRole == "viewer") {
            QMessageBox::information(this, "Режим доступа", "Вы вошли в режиме просмотра. Редактирование недоступно.");
        }

        client->fetchNotes();
        this->show();
        m_notesUpdateTimer->start(5000); // <-- ИЗМЕНЕНИЕ: Запускаем таймер на 5 секунд
    } else {
        QMessageBox::critical(this, "Ошибка", error.isEmpty() ? "Не удалось войти" : error);
        m_notesUpdateTimer->stop(); // <-- ИЗМЕНЕНИЕ: Останавливаем таймер при неудаче
        showLoginDialog();
    }
}

void MainWindow::onNotesReceived(const QJsonArray &notes)
{
    // Оптимизация: проверяем, изменились ли данные, прежде чем всё перерисовывать
    QJsonArray currentNotesArray;
    for(const QJsonObject& note : notesCache) {
        currentNotesArray.append(note);
    }
    if (currentNotesArray == notes) {
        return; // Если данные не изменились, выходим, чтобы избежать мерцания
    }

    notesCache.clear();

    QSqlQuery query(db);
    db.transaction();
    if (!query.exec("DELETE FROM notes")) {
        qWarning() << "Ошибка очистки локальной базы:" << query.lastError().text();
    }

    for (const auto& val : notes) {
        QJsonObject obj = val.toObject();
        int id = obj.value("id").toInt();
        notesCache[id] = obj;

        query.prepare("INSERT OR REPLACE INTO notes (id, date, hour, minute, note) VALUES (?, ?, ?, ?, ?)");
        query.addBindValue(id);
        query.addBindValue(obj.value("date").toString());
        query.addBindValue(obj.value("hour").toInt());
        query.addBindValue(obj.value("minute").toInt());
        query.addBindValue(obj.value("note").toString());
        if (!query.exec()) {
            qWarning() << "Ошибка вставки заметки в локальную базу:" << query.lastError().text();
        }
    }
    db.commit();

    loadNotesForDate(calendar->selectedDate());
    highlightDatesWithNotes();
    scheduleNotifications();
}

void MainWindow::onNoteAdded(const QJsonObject &note)
{
    // После добавления новой заметки просто запрашиваем полный список,
    // чтобы гарантировать консистентность данных
    client->fetchNotes();
}

void MainWindow::onNoteUpdated(const QJsonObject &note)
{
    // Аналогично, после обновления запрашиваем полный список
    client->fetchNotes();
}

void MainWindow::onNoteDeleted(int id)
{
    // И после удаления тоже
    client->fetchNotes();
}

void MainWindow::onErrorOccurred(const QString &error)
{
    QMessageBox::warning(this, "Ошибка", error);
}

void MainWindow::fillHoursList()
{
    list->clear();
    for (int hour=0; hour<24; ++hour)
        list->addItem(QString("%1:00").arg(hour,2,10,QChar('0')));

    loadNotesForDate(calendar->selectedDate());
}

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

void MainWindow::highlightDatesWithNotes()
{
    // Сначала сбрасываем все форматирования, кроме сегодняшней даты
    QMap<QDate, QTextCharFormat> savedFormats;
    QDate today = QDate::currentDate();
    savedFormats[today] = calendar->dateTextFormat(today);

    calendar->setDateTextFormat(QDate(), QTextCharFormat()); // очистить всё
    highlightCurrentDate(); // восстановить формат сегодняшней даты

    QTextCharFormat noteFormat;
    if (m_isDarkTheme) {
        noteFormat.setBackground(QBrush(Qt::darkGray));
        noteFormat.setForeground(QBrush(Qt::black));
    } else {
        noteFormat.setBackground(QBrush(QColor(Qt::lightGray)));
        noteFormat.setForeground(QBrush(Qt::black));
    }

    QSqlQuery query("SELECT DISTINCT date FROM notes");
    while(query.next()){
        QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
        if(date.isValid() && date != today) // не перезаписываем формат сегодняшнего дня
            calendar->setDateTextFormat(date, noteFormat);
    }
}

void MainWindow::loadNotesForDate(const QDate &date)
{
    for (int i = 0; i < list->count(); ++i) {
        QListWidgetItem* currentItem = list->item(i);
        // Сбрасываем текст, сохраняя только час
        currentItem->setText(QString("%1:00").arg(i, 2, 10, QChar('0')));
    }

    QMap<int, QList<QPair<int, QString>>> notesByHour;
    QSqlQuery query;
    query.prepare("SELECT hour, minute, note FROM notes WHERE date = ? ORDER BY minute ASC");
    query.addBindValue(date.toString(Qt::ISODate));
    if (!query.exec()) {
        qWarning() << "Ошибка SQL exec loadNotesForDate:" << query.lastError().text();
        return;
    }

    while (query.next()) {
        int hour = query.value(0).toInt();
        int minute = query.value(1).toInt();
        QString note = query.value(2).toString();
        notesByHour[hour].append(qMakePair(minute, note));
    }

    for (auto it=notesByHour.constBegin(); it!=notesByHour.constEnd(); ++it) {
        int hour = it.key();
        const QList<QPair<int, QString>>& noteList = it.value();

        QStringList noteStrings;
        for (const auto& p : noteList) {
            noteStrings.append(QString("%1:%2 - %3")
                                   .arg(hour, 2, 10, QChar('0'))
                                   .arg(p.first, 2, 10, QChar('0'))
                                   .arg(p.second));
        }
        QString combinedText = noteStrings.join(" | ");

        if(hour >=0 && hour < list->count()){
            list->item(hour)->setText(combinedText);
        }
    }
}

void MainWindow::saveNote(const QDate &date, int hour, int minute, const QString &text)
{
    QJsonObject noteJson;
    noteJson["date"] = date.toString(Qt::ISODate);
    noteJson["hour"] = hour;
    noteJson["minute"] = minute;
    noteJson["note"] = text.trimmed();
    noteJson["shared_with"] = QJsonArray(); // пустой массив для совместного использования (если не используется)

    int existingId = -1;
    for(auto it = notesCache.begin(); it != notesCache.end(); ++it){
        const QJsonObject& obj = it.value();
        if (obj.value("date").toString() == date.toString(Qt::ISODate) &&
            obj.value("hour").toInt() == hour &&
            obj.value("minute").toInt() == minute){
            existingId = it.key();
            break;
        }
    }
    if (existingId == -1) {
        client->addNote(noteJson);
    } else {
        client->updateNote(existingId, noteJson);
    }
}

void MainWindow::deleteNoteById(int id)
{
    if (id == -1) return;

    client->deleteNote(id);
}

void MainWindow::onDateChanged()
{
    loadNotesForDate(calendar->selectedDate());
}

void MainWindow::onHourDoubleClicked(QListWidgetItem* item)
{
    if (m_currentUserRole == "viewer") {
        QMessageBox::information(this, "Доступ запрещен", "В режиме просмотра нельзя добавлять или изменять заметки.");
        return; // Выходим, не давая редактировать
    }

    int hour = list->row(item);
    QDate selectedDate = calendar->selectedDate();

    bool okMinute;
    int minute = QInputDialog::getInt(this, "Выбор минуты",
                                      QString("Выберите минуту для %1:").arg(hour, 2, 10, QChar('0')),
                                      0, 0, 59, 1, &okMinute);
    if (!okMinute)
        return;

    QString currentNote;
    int existingId = -1;
    for (auto it=notesCache.begin(); it!=notesCache.end(); ++it) {
        const QJsonObject& obj = it.value();
        if(obj.value("date").toString() == selectedDate.toString(Qt::ISODate) &&
            obj.value("hour").toInt() == hour &&
            obj.value("minute").toInt() == minute)
        {
            currentNote = obj.value("note").toString();
            existingId = it.key();
            break;
        }
    }

    bool okText;
    QString text = QInputDialog::getText(this,
                                         QString("Заметка на %1 %2:%3").arg(selectedDate.toString()).arg(hour, 2, 10, QChar('0')).arg(minute, 2, 10, QChar('0')),
                                         "Введите заметку:",
                                         QLineEdit::Normal,
                                         currentNote,
                                         &okText);
    if (!okText)
        return;

    if (text.trimmed().isEmpty() && existingId != -1)
        deleteNoteById(existingId);
    else if (!text.trimmed().isEmpty())
        saveNote(selectedDate, hour, minute, text);
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

    QSqlQuery query("SELECT date, hour, minute, note FROM notes");
    while (query.next()) {
        QDate date = QDate::fromString(query.value(0).toString(), Qt::ISODate);
        int hour = query.value(1).toInt();
        int minute = query.value(2).toInt();
        QString note = query.value(3).toString();
        scheduleNotificationFor(date, hour, minute, note);
    }
}

void MainWindow::scheduleNotificationFor(const QDate &date, int hour, int minute, const QString &note)
{
    QDateTime noteDateTime(date, QTime(hour, minute));
    QDateTime notifyTime = noteDateTime.addSecs(-30 * 60); // за 30 минут

    QDateTime now = QDateTime::currentDateTime();

    if (notifyTime <= now)
        return;

    qint64 msecToNotify = now.msecsTo(notifyTime);
    if (msecToNotify <= 0)
        return;

    QString key = date.toString(Qt::ISODate) + "_" + QString::number(hour) + "_" + QString::number(minute);

    QTimer* timer = new QTimer(this);
    timer->setSingleShot(true);

    connect(timer, &QTimer::timeout, this, &MainWindow::showNotification);

    notificationTimers[key] = timer;

    timer->setProperty("noteText", note);
    timer->setProperty("noteDateTime", noteDateTime);

    timer->start(msecToNotify);
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

    highlightDatesWithNotes();
    highlightCurrentDate();
    loadNotesForDate(calendar->selectedDate());
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

    highlightDatesWithNotes();
    highlightCurrentDate();
    loadNotesForDate(calendar->selectedDate());
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

void MainWindow::saveTheme(const QString &themeName)
{
    QSettings settings("MyCompany", "MyApp");
    settings.setValue("theme", themeName);
}

void MainWindow::changeTheme(QAction *action)
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
