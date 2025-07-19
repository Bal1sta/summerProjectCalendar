#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);

    void login(const QString &username, const QString &password);
    void registerUser(const QString &username, const QString &password);

    bool isConnected() const;                               // Обязательна!

    void sendMessage(const QJsonObject &message);           // Обязательна!

    void deleteEvent(int id);

signals:
    void connected();
    void disconnected();
    void loginResult(bool success);
    void registrationResult(bool success, const QString &reason = QString());

    void eventUpdated(const QJsonObject &event);
    void eventDeleted(int id);

private slots:
    void onReadyRead();
    void onStateChanged(QAbstractSocket::SocketState state);

private:
    QTcpSocket *m_socket;
    QByteArray m_buffer;
};

#endif // CLIENT_H
