#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QSet>
#include <QJsonDocument>
#include <QJsonObject>

struct Event {
    int id;
    QString date;
    int hour;
    int minute;
    QString note;
    QString owner;
    QSet<QString> editors;
    QSet<QString> viewers;
};

class MyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit MyServer(QObject *parent = nullptr);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QMap<qintptr, QTcpSocket*> clients;
    QMap<QString, QString> users;
    QMap<qintptr, QString> clientLogins;
    QMap<int, Event> events;
    int eventIdCounter = 0;

    QMap<qintptr, QByteArray> buffers;

    void processMessage(const QString& login, const QJsonObject &msg, qintptr socketId);
    void broadcastEventUpdate(const Event& event);
};

#endif // MYSERVER_H
