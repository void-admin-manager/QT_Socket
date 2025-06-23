#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QCryptographicHash>
#include <QDataStream>
#include <QMap>
#include <QThread>
#include <QList>

class ClientHandler : public QObject{
    Q_OBJECT

public:
    explicit ClientHandler(qintptr handle, const QString &filePath, QObject *parent = nullptr);

public slots:
    void readClient();

private:
    QTcpSocket *socket;
    QString filePath;
    QByteArray fileMD5;

signals:
    void finished();

private slots:
    void sendFile();
    void handleError(QAbstractSocket::SocketError error);
};

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(const QString &filePath, QObject *parent = nullptr);

private:
    QString filePath;

private slots:

signals:

protected:
    void incomingConnection(qintptr handle) override;
};

#endif // SERVER_H
