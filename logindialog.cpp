#include "logindialog.h"
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

QString LoginDialog::user() const {
    return usernameEdit->text();
}

QString LoginDialog::pass() const {
    return passwordEdit->text();
}

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Авторизация");
    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Логин:", this));
    usernameEdit = new QLineEdit(this);
    layout->addWidget(usernameEdit);

    layout->addWidget(new QLabel("Пароль:", this));
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    layout->addWidget(passwordEdit);

    loginButton = new QPushButton("Войти", this);
    registerButton = new QPushButton("Регистрация", this);

    layout->addWidget(loginButton);
    layout->addWidget(registerButton);

    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::onRegisterClicked);
}

void LoginDialog::onLoginClicked()
{
    if (usernameEdit->text().trimmed().isEmpty() || passwordEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Введите логин и пароль");
        return;
    }
    emit loginRequested(usernameEdit->text().trimmed(), passwordEdit->text());
}

void LoginDialog::onRegisterClicked()
{
    emit registerRequested();
}
