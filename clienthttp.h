#ifndef CLIENTHTTP_H
#define CLIENTHTTP_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QVariant>
#include <QJsonArray>

class ClientHttp : public QObject
{
    Q_OBJECT
public:
    explicit ClientHttp(QObject *parent = nullptr);

    void setServerUrl(const QString &url);

    // Геттер теперь возвращает приватный сервер URL
    QString serverUrl() const { return m_serverUrl; }

    void login(const QString &username, const QString &password);
    void registerUser(const QString &username, const QString &password);

    void fetchNotes();
    void addNote(const QJsonObject &note);
    void updateNote(int id, const QJsonObject &note);
    void deleteNote(int id);

signals:
    void loginResult(bool success, const QString &role, const QString &error = QString());
    void registerResult(bool success, const QString &msg = QString());
    void notesReceived(const QJsonArray &notes);
    void noteAdded(const QJsonObject &note);
    void noteUpdated(const QJsonObject &note);
    void noteDeleted(int id);
    void errorOccurred(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    QString m_serverUrl;
    QString accessToken;
    QString m_userRole; // Поле для хранения роли
    QNetworkAccessManager* networkManager;
    QHash<QNetworkReply*, QString> pendingRequests;
    QHash<QNetworkReply*, QVariant> pendingData;
};

#endif // CLIENTHTTP_H
