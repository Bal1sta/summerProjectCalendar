#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;

class RegisterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RegisterDialog(QWidget *parent = nullptr);

signals:
    void registrationRequested(const QString &username, const QString &password);

private slots:
    void onRegisterClicked();

private:
    QLineEdit *usernameEdit;
    QLineEdit *passwordEdit;
    QLineEdit *passwordConfirmEdit;
    QPushButton *registerButton;
    QPushButton *cancelButton;
};

#endif // REGISTERDIALOG_H
