#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H
#include <QDialog>
class QLineEdit;
class QPushButton;

class LoginDialog : public QDialog {
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = nullptr);

    QString user() const;
    QString pass() const;

signals:
    void loginRequested(const QString &username, const QString &password);
    void registerRequested();
private slots:
    void onLoginClicked();
    void onRegisterClicked();
private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QPushButton *registerButton;
};
#endif // LOGINDIALOG_H
