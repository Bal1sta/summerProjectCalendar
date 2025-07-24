#include "registerdialog.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

RegisterDialog::RegisterDialog(QWidget *parent): QDialog(parent)
{
    setWindowTitle("Регистрация");

    auto vbox = new QVBoxLayout(this);
    auto form = new QFormLayout();

    usernameEdit = new QLineEdit(this);
    passwordEdit = new QLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordConfirmEdit = new QLineEdit(this);
    passwordConfirmEdit->setEchoMode(QLineEdit::Password);

    form->addRow("Логин:", usernameEdit);
    form->addRow("Пароль:", passwordEdit);
    form->addRow("Повтор пароля:", passwordConfirmEdit);

    vbox->addLayout(form);

    registerButton = new QPushButton("Зарегистрироваться", this);
    cancelButton = new QPushButton("Отмена", this);

    vbox->addWidget(registerButton);
    vbox->addWidget(cancelButton);

    connect(registerButton, &QPushButton::clicked, this, &RegisterDialog::onRegisterClicked);
    connect(cancelButton, &QPushButton::clicked, this, &RegisterDialog::reject);
}

void RegisterDialog::onRegisterClicked()
{
    QString user = usernameEdit->text().trimmed();
    QString pass = passwordEdit->text();
    QString passConfirm = passwordConfirmEdit->text();

    if(user.isEmpty() || pass.isEmpty() || passConfirm.isEmpty()){
        QMessageBox::warning(this, "Ошибка", "Заполните все поля");
        return;
    }

    if(pass != passConfirm){
        QMessageBox::warning(this, "Ошибка", "Пароли не совпадают");
        return;
    }

    emit registrationRequested(user, pass);
}
