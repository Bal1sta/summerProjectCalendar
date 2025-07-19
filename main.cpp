#include <QApplication>
#include "client.h"
#include "logindialog.h"
#include "registerdialog.h"
#include "mainwindow.h"
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    Client client;
    client.connectToServer("127.0.0.1", 12345);

    LoginDialog loginDlg;

    bool regDialogOpen = false;

    QObject::connect(&loginDlg, &LoginDialog::loginRequested, [&](const QString &user, const QString &pass){
        client.login(user, pass);
    });

    QObject::connect(&loginDlg, &LoginDialog::registerRequested, [&](){
        if(regDialogOpen)
            return;

        regDialogOpen = true;
        loginDlg.hide();

        RegisterDialog regDlg;

        QObject::connect(&regDlg, &RegisterDialog::registrationRequested, [&](const QString &user, const QString &pass){
            client.registerUser(user, pass);
        });

        QObject::connect(&client, &Client::registrationResult, [&](bool success, const QString &reason){
            if(!regDlg.isVisible())
                return;

            if(success) {
                QMessageBox::information(nullptr, "Регистрация", "Регистрация успешна! Теперь вы можете войти.");
                regDlg.accept();
            } else {
                QMessageBox::warning(nullptr, "Регистрация", "Ошибка регистрации: " + reason);
            }
        });

        regDlg.exec();

        regDialogOpen = false;
        loginDlg.show();
    });

    QObject::connect(&client, &Client::loginResult, [&](bool success){
        if(success) {
            loginDlg.accept();
        } else {
            QMessageBox::warning(nullptr, "Ошибка", "Неверный логин или пароль");
        }
    });

    if(loginDlg.exec() != QDialog::Accepted)
        return 0;

    MainWindow w;
    w.show();

    return a.exec();
}
