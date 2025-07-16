#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QActionGroup>
#include <QSettings>
#include <QMenu>

class ThemeManager : public QObject
{
    Q_OBJECT
public:
    explicit ThemeManager(QObject *parent = nullptr);

    enum Theme {
        Light,
        Dark
    };

    void setupThemeMenu(QMenu *menu);
    void applySavedTheme();
    Theme currentTheme() const { return m_currentTheme; }

signals:
    void themeChanged(ThemeManager::Theme newTheme);

public slots:
    void changeTheme(QAction *action);

private:
    void applyTheme(Theme theme);
    void saveTheme(Theme theme);

    QActionGroup *m_actionGroup;
    QSettings m_settings;
    Theme m_currentTheme;
};

#endif // THEMEMANAGER_H
