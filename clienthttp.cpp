#include "ClientHttp.h"
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QJsonArray>

ClientHttp::ClientHttp(QObject *parent)
    : QObject(parent),
    networkManager(new QNetworkAccessManager(this))
{
    connect(networkManager, &QNetworkAccessManager::finished, this, &ClientHttp::onReplyFinished);
}

void ClientHttp::setServerUrl(const QString &url)
{
    m_serverUrl = url;
}

void ClientHttp::login(const QString &username, const QString &password)
{
    if (m_serverUrl.isEmpty()) {
        emit errorOccurred("URL сервера не задан");
        emit loginResult(false, "", "URL сервера не задан");
        return;
    }

    QUrl url(m_serverUrl + "/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QByteArray postData;
    postData.append("username=" + QUrl::toPercentEncoding(username));
    postData.append("&password=" + QUrl::toPercentEncoding(password));

    QNetworkReply* reply = networkManager->post(request, postData);
    pendingRequests[reply] = "login";
}

void ClientHttp::registerUser(const QString &username, const QString &password)
{
    if (m_serverUrl.isEmpty()) {
        emit errorOccurred("URL сервера не задан");
        emit registerResult(false, "URL сервера не задан");
        return;
    }
    QUrl url(m_serverUrl + "/register");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    QByteArray postData = QJsonDocument(body).toJson();
    QNetworkReply* reply = networkManager->post(request, postData);
    pendingRequests[reply] = "register";
}

void ClientHttp::fetchNotes()
{
    if (accessToken.isEmpty()) {
        emit errorOccurred("Не выполнен вход");
        return;
    }
    QUrl url(m_serverUrl + "/notes");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    QNetworkReply* reply = networkManager->get(request);
    pendingRequests[reply] = "fetchNotes";
}

void ClientHttp::addNote(const QJsonObject &note)
{
    if (accessToken.isEmpty()) {
        emit errorOccurred("Не выполнен вход");
        return;
    }
    QUrl url(m_serverUrl + "/notes");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    QByteArray data = QJsonDocument(note).toJson();
    QNetworkReply* reply = networkManager->post(request, data);
    pendingRequests[reply] = "addNote";
    pendingData[reply] = note;
}

void ClientHttp::updateNote(int id, const QJsonObject &note)
{
    if (accessToken.isEmpty()) {
        emit errorOccurred("Не выполнен вход");
        return;
    }
    QUrl url(m_serverUrl + "/notes/" + QString::number(id));
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    QByteArray data = QJsonDocument(note).toJson();
    QNetworkReply* reply = networkManager->put(request, data);
    pendingRequests[reply] = "updateNote";
    pendingData[reply] = id;
}

void ClientHttp::deleteNote(int id)
{
    if (accessToken.isEmpty()) {
        emit errorOccurred("Не выполнен вход");
        return;
    }
    QUrl url(m_serverUrl + "/notes/" + QString::number(id));
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + accessToken.toUtf8());
    QNetworkReply* reply = networkManager->deleteResource(request);
    pendingRequests[reply] = "deleteNote";
    pendingData[reply] = id;
}

void ClientHttp::onReplyFinished(QNetworkReply* reply)
{
    if (!pendingRequests.contains(reply)) {
        reply->deleteLater();
        return;
    }

    QString reqType = pendingRequests.take(reply);
    QVariant extraData = pendingData.take(reply);

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        QByteArray body = reply->readAll();
        auto json = QJsonDocument::fromJson(body);
        if (json.isObject() && json.object().contains("detail"))
            err += ": " + json.object().value("detail").toString();
        emit errorOccurred(err);

        if (reqType == "login")
            emit loginResult(false, "", err);
        else if (reqType == "register")
            emit registerResult(false, err);

        reply->deleteLater();
        return;
    }

    QByteArray resp = reply->readAll();

    if (reqType == "register") {
        auto json = QJsonDocument::fromJson(resp);
        if (json.isObject() && json.object().contains("msg")) {
            emit registerResult(true, json.object().value("msg").toString());
        } else {
            emit registerResult(false, "Ошибка регистрации");
        }
    }
    else if (reqType == "login") {
        auto json = QJsonDocument::fromJson(resp);
        if (json.isObject()) {
            auto obj = json.object();
            QString token = obj.value("access_token").toString();
            QString role = obj.value("user_role").toString(); // ИЗВЛЕКАЕМ РОЛЬ

            if (!token.isEmpty() && !role.isEmpty()) {
                accessToken = token;
                m_userRole = role; // СОХРАНЯЕМ РОЛЬ
                emit loginResult(true, m_userRole); // ПЕРЕДАЕМ РОЛЬ
            } else {
                emit loginResult(false, "", "Нет токена или роли в ответе");
            }
        } else {
            emit loginResult(false, "", "Некорректный ответ сервера");
        }
    }
    else if (reqType == "fetchNotes") {
        auto json = QJsonDocument::fromJson(resp);
        if (json.isArray()) {
            emit notesReceived(json.array());
        } else {
            emit errorOccurred("Некорректный формат заметок от сервера");
        }
    }
    else if (reqType == "addNote") {
        auto json = QJsonDocument::fromJson(resp);
        if (json.isObject()) {
            emit noteAdded(json.object());
        }
    }
    else if (reqType == "updateNote") {
        auto json = QJsonDocument::fromJson(resp);
        if (json.isObject()) {
            emit noteUpdated(json.object());
        }
    }
    else if (reqType == "deleteNote") {
        int id = extraData.toInt();
        emit noteDeleted(id);
    }

    reply->deleteLater();
}
