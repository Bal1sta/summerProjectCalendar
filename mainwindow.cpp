#include "mainwindow.h"
#include <QHBoxLayout>
#include <QInputDialog>
#include <QTextCharFormat>
#include <QBrush>
#include <QColor>
#include <QDate>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget* centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #808080;");  // серый цвет
    setCentralWidget(centralWidget);

    calendar = new QCalendarWidget(this);
    calendar->setStyleSheet(R"(
        QCalendarWidget QWidget {
            background-color: #232629;
            color: #f0f0f0;
        }
        QCalendarWidget QAbstractItemView {
            background-color: #232629;
            color: #f0f0f0;
            selection-background-color: #3d4250;
            selection-color: #ffffff;
        }
        QCalendarWidget QToolButton {
            background-color: #232629;
            color: #f0f0f0;
        }
        QCalendarWidget QSpinBox {
            background-color: #232629;
            color: #f0f0f0;
        }
        QCalendarWidget QMenu {
            background-color: #232629;
            color: #f0f0f0;
        }
    )");


    list = new QListWidget(this);
    list->setStyleSheet(R"(
    QListWidget {
        background-color: #232629;
        color: #f0f0f0;
        border: 1px solid #3d4250;
    }
    QListWidget::item {
        background-color: #232629;
        color: #f0f0f0;
    }
    QListWidget::item:selected {
        background-color: #3d4250;
        color: #ffffff;
    }
    )");

    QHBoxLayout* layout = new QHBoxLayout(centralWidget);
    layout->addWidget(calendar);
    layout->addWidget(list);

    fillHoursList();
    highlightCurrentDate();  // Подсвечиваем текущую дату сразу при запуске
    highlightDatesWithNotes(); // Подсвечиваем даты с заметками (если есть)

    connect(calendar, &QCalendarWidget::selectionChanged, this, &MainWindow::onDateChanged);
    connect(list, &QListWidget::itemDoubleClicked, this, &MainWindow::onHourDoubleClicked);

    setMinimumSize(800, 600);
    list->setItemDelegate(new ItemDelegate(list));
}

MainWindow::~MainWindow()
{
}

void MainWindow::fillHoursList()
{
    list->clear();
    for (int hour = 0; hour < 24; ++hour) {
        list->addItem(QString("%1:00").arg(hour, 2, 10, QChar('0')));
    }
}

void MainWindow::highlightCurrentDate()
{
    QTextCharFormat format;
    format.setBackground(QBrush(QColor(0, 122, 204))); // синий цвет фона
    format.setForeground(QBrush(Qt::black));          // цвет текста

    QDate today = QDate::currentDate();
    calendar->setDateTextFormat(today, format);
}

void MainWindow::highlightDatesWithNotes()
{
    // Сбрасываем формат для всех дат
    calendar->setDateTextFormat(QDate(), QTextCharFormat());

    // Снова подсвечиваем текущую дату, чтобы не потерять выделение
    highlightCurrentDate();

    // Формат для дат с заметками
    QTextCharFormat noteFormat;
    noteFormat.setBackground(QBrush(QColor(QColorConstants::DarkGray)));
    noteFormat.setForeground(QBrush(Qt::black));

    for (const QDate& date : notes.keys()) {
        if (!notes[date].isEmpty()) {
            calendar->setDateTextFormat(date, noteFormat);
        }
    }
}

void MainWindow::onDateChanged()
{
    QDate selectedDate = calendar->selectedDate();
    fillHoursList();

    if (notes.contains(selectedDate)) {
        const auto& dayNotes = notes[selectedDate];
        for (int i = 0; i < list->count(); ++i) {
            int hour = i;
            if (dayNotes.contains(hour)) {
                list->item(i)->setText(QString("%1:00 - %2").arg(hour, 2, 10, QChar('0')).arg(dayNotes[hour]));
            }
        }
    }
}

void MainWindow::onHourDoubleClicked(QListWidgetItem* item)
{
    int hour = list->row(item);
    QDate selectedDate = calendar->selectedDate();

    QString currentNote;
    if (notes.contains(selectedDate) && notes[selectedDate].contains(hour)) {
        currentNote = notes[selectedDate][hour];
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
            // Удаляем заметку, если текст пустой
            if (notes.contains(selectedDate)) {
                notes[selectedDate].remove(hour);
                if (notes[selectedDate].isEmpty()) {
                    notes.remove(selectedDate);
                }
            }
            item->setText(QString("%1:00").arg(hour, 2, 10, QChar('0')));
        } else {
            // Сохраняем заметку
            notes[selectedDate][hour] = text;
            item->setText(QString("%1:00 - %2").arg(hour, 2, 10, QChar('0')).arg(text));
        }
        highlightDatesWithNotes(); // Обновляем подсветку дат с заметками
    }
}
