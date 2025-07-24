#include "thememanager.h"
#include <QApplication>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent),
    m_actionGroup(new QActionGroup(this)),
    m_settings("MyCompany", "MyApp"),
    m_currentTheme(Dark)
{
}

void ThemeManager::setupThemeMenu(QMenu *menu)
{
<<<<<<< HEAD
=======
    // Обязательно подключен <QMenu>

>>>>>>> 33978bc2a58cb07d16ba6eb66d32f282a4862a9b
    QAction *lightThemeAction = new QAction(tr("Светлая"), menu);
    QAction *darkThemeAction = new QAction(tr("Тёмная"), menu);

    lightThemeAction->setCheckable(true);
    darkThemeAction->setCheckable(true);

    m_actionGroup->addAction(lightThemeAction);
    m_actionGroup->addAction(darkThemeAction);

    menu->addAction(lightThemeAction);
    menu->addAction(darkThemeAction);

    connect(m_actionGroup, &QActionGroup::triggered, this, &ThemeManager::changeTheme);

    // Загрузка сохранённой темы
    QString savedTheme = m_settings.value("theme", "Dark").toString();
    if (savedTheme == "Light") {
        lightThemeAction->setChecked(true);
        applyTheme(Light);
    } else {
        darkThemeAction->setChecked(true);
        applyTheme(Dark);
    }
}

void ThemeManager::applySavedTheme()
{
    QString savedTheme = m_settings.value("theme", "Dark").toString();
    if (savedTheme == "Light")
        applyTheme(Light);
    else
        applyTheme(Dark);
}

void ThemeManager::changeTheme(QAction *action)
{
    if (!action)
        return;

    if (action->text() == tr("Светлая")) {
        applyTheme(Light);
    } else if (action->text() == tr("Тёмная")) {
        applyTheme(Dark);
    }
}

void ThemeManager::applyTheme(Theme theme)
{
    m_currentTheme = theme;

    switch (theme) {
    case Light:
        qApp->setStyleSheet(R"(
            QMainWindow { background-color: #FFFFFF; color: #000000; }
            QCalendarWidget QWidget { background-color: #FFFFFF; color: #000000; }
            QCalendarWidget QAbstractItemView { background-color: #FFFFFF; color: #000000; selection-background-color: #cce8ff; selection-color: #000000; }
            QCalendarWidget QToolButton { background-color: #f0f0f0; color: #000000; }
            QCalendarWidget QSpinBox { background-color: #f0f0f0; color: #000000; }
            QListWidget { background-color: #FFFFFF; color: #000000; border: 1px solid #cccccc; }
            QListWidget::item:selected { background-color: #cce8ff; color: #000000; }
        )");
        break;
    case Dark:
        qApp->setStyleSheet(R"(
            QMainWindow { background-color: #232629; color: #f0f0f0; }
            QCalendarWidget QWidget { background-color: #232629; color: #f0f0f0; }
            QCalendarWidget QAbstractItemView { background-color: #232629; color: #f0f0f0; selection-background-color: #3d4250; selection-color: #f0f0f0; }
            QCalendarWidget QToolButton { background-color: #232629; color: #f0f0f0; }
            QCalendarWidget QSpinBox { background-color: #232629; color: #f0f0f0; }
            QListWidget { background-color: #232629; color: #f0f0f0; border: 1px solid #3d4250; }
            QListWidget::item:selected { background-color: #3d4250; color: #ffffff; }
        )");
        break;
    }

    saveTheme(theme);

    emit themeChanged(theme);
}

void ThemeManager::saveTheme(Theme theme)
{
    QString themeStr = (theme == Light) ? "Light" : "Dark";
    m_settings.setValue("theme", themeStr);
}
