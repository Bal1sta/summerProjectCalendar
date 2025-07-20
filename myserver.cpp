#include "myserver.h"
#include <QJsonArray>
#include <QDebug>

MyServer::MyServer(QObject *parent) : QTcpServer(parent)
{
    // Для примера: 2 пользователя
    users["alice"] = "pass1";
    users["bob"] = "pass2";
}

void MyServer::incomingConnection(qintptr socketDescriptor)
{
    QTcpSocket* clientSocket = new QTcpSocket(this);
    clientSocket->setSocketDescriptor(socketDescriptor);
    clients[socketDescriptor] = clientSocket;

    connect(clientSocket, &QTcpSocket::readyRead, this, &MyServer::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &MyServer::onDisconnected);

    qDebug() << "Client connected:" << socketDescriptor;
}

void MyServer::onReadyRead()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if(!clientSocket) return;

    qintptr socketId = clientSocket->socketDescriptor();
    QByteArray& buf = buffers[socketId];

    buf.append(clientSocket->readAll());

    while (true) {
        int idx = buf.indexOf('\n');
        if (idx == -1) break;

        QByteArray line = buf.left(idx).trimmed();
        buf.remove(0, idx + 1);

        if (line.isEmpty())
            continue;

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError) {
            qDebug() << "JSON parse error:" << err.errorString();
            continue;
        }
        if (!doc.isObject())
            continue;

        QJsonObject msg = doc.object();
        QString login = clientLogins.value(socketId);

        processMessage(login, msg, socketId);
    }
}

void MyServer::processMessage(const QString& login, const QJsonObject &msg, qintptr socketId)
{
    QTcpSocket* clientSocket = clients.value(socketId);
    if(!clientSocket) return;

    QString type = msg.value("type").toString();

    if(type == "login") {
        QString user = msg.value("user").toString();
        QString pass = msg.value("pass").toString();

        if(users.contains(user) && users[user] == pass) {
            clientLogins[socketId] = user;

            QJsonObject resp;
            resp["type"] = "login_result";
            resp["status"] = "success";
            QByteArray data = QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n";
            clientSocket->write(data);
            clientSocket->flush();
            qDebug() << user << "logged in";

            // Отправляем события
            QJsonArray arr;
            for(const Event& e : events) {
                if(e.owner == user || e.viewers.contains(user)) {
                    QJsonObject ev;
                    ev["id"] = e.id;
                    ev["date"] = e.date;
                    ev["hour"] = e.hour;
                    ev["minute"] = e.minute;
                    ev["note"] = e.note;
                    arr.append(ev);
                }
            }
            QJsonObject evmsg;
            evmsg["type"] = "events_list";
            evmsg["events"] = arr;
            QByteArray dataEvents = QJsonDocument(evmsg).toJson(QJsonDocument::Compact) + "\n";
            clientSocket->write(dataEvents);
            clientSocket->flush();
        } else {
            QJsonObject resp;
            resp["type"] = "login_result";
            resp["status"] = "fail";
            QByteArray data = QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n";
            clientSocket->write(data);
            clientSocket->flush();
        }
        return;
    }

    if(type == "register") {
        QString user = msg.value("user").toString();
        QString pass = msg.value("pass").toString();

        QJsonObject resp;
        resp["type"] = "register_result";

        if(users.contains(user)) {
            resp["status"] = "fail";
            resp["reason"] = "Пользователь уже существует";
        } else {
            users[user] = pass;
            resp["status"] = "success";
        }

        QByteArray data = QJsonDocument(resp).toJson(QJsonDocument::Compact) + "\n";
        clientSocket->write(data);
        clientSocket->flush();
        return;
    }

    if(login.isEmpty()) {
        // Не авторизован - игнорируем
        return;
    }

    if(type == "create_event") {
        QString date = msg.value("date").toString();
        int hour = msg.value("hour").toInt();
        int minute = msg.value("minute").toInt();
        QString note = msg.value("note").toString();

        Event e;
        e.id = ++eventIdCounter;
        e.date = date;
        e.hour = hour;
        e.minute = minute;
        e.note = note;
        e.owner = login;
        e.editors.insert(login);
        e.viewers.insert(login);

        events[e.id] = e;

        broadcastEventUpdate(e);
    }
    else if(type == "edit_event") {
        int id = msg.value("id").toInt();
        if(!events.contains(id))
            return;

        Event &e = events[id];

        if(!e.editors.contains(login))
            return;

        if(msg.contains("date")) e.date = msg.value("date").toString();
        if(msg.contains("hour")) e.hour = msg.value("hour").toInt();
        if(msg.contains("minute")) e.minute = msg.value("minute").toInt();
        if(msg.contains("note")) e.note = msg.value("note").toString();

        broadcastEventUpdate(e);
    }
    else if(type == "delete_event") {
        int id = msg.value("id").toInt();
        if(!events.contains(id))
            return;
        Event &e = events[id];
        if(e.owner != login)
            return;
        events.remove(id);

        QJsonObject delMsg;
        delMsg["type"] = "delete_event";
        delMsg["id"] = id;

        QByteArray data = QJsonDocument(delMsg).toJson(QJsonDocument::Compact) + "\n";

        for(auto socket : clients) {
            socket->write(data);
            socket->flush();
        }
    }
    else if(type == "share_event") {
        int id = msg.value("id").toInt();
        if(!events.contains(id))
            return;
        Event &e = events[id];
        if(e.owner != login)
            return;

        QString targetUser = msg.value("user").toString();
        QString right = msg.value("right").toString();

        if(!users.contains(targetUser))
            return;

        e.viewers.insert(targetUser);
        if(right == "edit")
            e.editors.insert(targetUser);

        broadcastEventUpdate(e);
    }
}

void MyServer::broadcastEventUpdate(const Event& event)
{
    QJsonObject ev;
    ev["type"] = "update_event";
    ev["id"] = event.id;
    ev["date"] = event.date;
    ev["hour"] = event.hour;
    ev["minute"] = event.minute;
    ev["note"] = event.note;
    ev["owner"] = event.owner;

    QJsonArray editors;
    for(const QString& ed : event.editors)
        editors.append(ed);
    ev["editors"] = editors;

    QJsonArray viewers;
    for(const QString& v : event.viewers)
        viewers.append(v);
    ev["viewers"] = viewers;

    QByteArray data = QJsonDocument(ev).toJson(QJsonDocument::Compact) + "\n";

    for(auto it = clients.begin(); it != clients.end(); ++it) {
        qintptr socketId = it.key();
        QString login = clientLogins.value(socketId);
        if(event.owner == login || event.viewers.contains(login)) {
            it.value()->write(data);
            it.value()->flush();
        }
    }
}

void MyServer::onDisconnected()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    if(!clientSocket) return;
    qintptr socketId = clientSocket->socketDescriptor();

    clients.remove(socketId);
    clientLogins.remove(socketId);
    buffers.remove(socketId);

    clientSocket->deleteLater();

    qDebug() << "Client disconnected:" << socketId;
}
