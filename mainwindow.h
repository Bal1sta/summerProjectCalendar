#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCalendarWidget>
#include <QListWidget>
#include <QMap>
#include <QDate>
#include <QStyledItemDelegate>

class ItemDelegate : public QStyledItemDelegate {
public:
    explicit ItemDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        QSize size = QStyledItemDelegate::sizeHint(option, index);
        size.setHeight(size.height() + 10); // увеличиваем высоту элемента на 10 пикселей
        return size;
    }
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QCalendarWidget* calendar;
    QListWidget* list;

    // Структура для хранения заметок: дата -> (час -> текст заметки)
    QMap<QDate, QMap<int, QString>> notes;

    void fillHoursList();
    void highlightCurrentDate();  // Метод для подсветки текущей даты
    void highlightDatesWithNotes(); // Метод для подсветки дат с заметками

private slots:
    void onDateChanged();
    void onHourDoubleClicked(QListWidgetItem* item);
};

#endif // MAINWINDOW_H
