#include "client.h"
#include <QJsonDocument>
#include <QDebug>

Client::Client(QObject *parent) : QObject(parent),
    m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(m_socket, &QTcpSocket::stateChanged, this, &Client::onStateChanged);
}

void Client::connectToServer(const QString &host, quint16 port)
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
        m_socket->disconnectFromHost();

    m_socket->connectToHost(host, port);
}

bool Client::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void Client::sendMessage(const QJsonObject &message)
{
    if (!isConnected()) {
        qWarning() << "Not connected to server";
        return;
    }

    QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
    m_socket->write(data);
}

void Client::login(const QString &username, const QString &password)
{
    QJsonObject msg;
    msg["type"] = "login";
    msg["user"] = username;
    msg["pass"] = password;
    sendMessage(msg);
}

void Client::registerUser(const QString &username, const QString &password)
{
    QJsonObject msg;
    msg["type"] = "register";
    msg["user"] = username;
    msg["pass"] = password;
    sendMessage(msg);
}

void Client::deleteEvent(int id)
{
    QJsonObject msg;
    msg["type"] = "delete_event";
    msg["id"] = id;
    sendMessage(msg);
}

void Client::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while (true) {
        int idx = m_buffer.indexOf('\n');
        if (idx == -1) {
            // Ждём завершения сообщения
            break;
        }

        QByteArray line = m_buffer.left(idx).trimmed();
        m_buffer.remove(0, idx + 1);

        if (line.isEmpty())
            continue;

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(line, &error);
        if (error.error != QJsonParseError::NoError) {
            qWarning() << "JSON parse error:" << error.errorString() << ", data:" << line;
            continue;
        }

        QJsonObject obj = doc.object();

        QString type = obj.value("type").toString();

        if (type == "login_result") {
            bool success = obj.value("status").toString() == "success";
            emit loginResult(success);
        }
        else if (type == "register_result") {
            bool success = obj.value("status").toString() == "success";
            QString reason = obj.value("reason").toString();
            emit registrationResult(success, reason);
        }
        else if (type == "events_list") {
            // Можно обработать список событий, если требуется
        }
        // Другие типы сообщений можно здесь добавить по необходимости
    }
}

void Client::onStateChanged(QAbstractSocket::SocketState state)
{
    if (state == QAbstractSocket::ConnectedState)
        emit connected();
    else if (state == QAbstractSocket::UnconnectedState)
        emit disconnected();
}
