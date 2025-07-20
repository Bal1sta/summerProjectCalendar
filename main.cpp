// main.cpp
#include <QCoreApplication>
#include "myserver.h"
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    MyServer server;
    if (!server.listen(QHostAddress::Any, 12345)) {
        qCritical() << "Failed to start server:" << server.errorString();
        return 1;
    }
    qDebug() << "Server started on port 12345";

    return a.exec();
}
