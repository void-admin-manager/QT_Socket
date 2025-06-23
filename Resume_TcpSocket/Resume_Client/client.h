#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QCoreApplication>
#include <QTcpSocket>
#include <QFile>
#include <QCryptographicHash>
#include <QDebug>

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(const QString &host, quint16 port,
                    const QString &savePath, QObject *parent = nullptr);

private slots:
    void requestFile();
    void readData();
    void verifyFile();
    void handleError(QAbstractSocket::SocketError error);

private:
    QTcpSocket *socket;
    QString host;
    quint16 port;
    QString savePath;

    QFile file;
    qint64 fileSize = 0;
    qint64 receivedSize = 0;
    QByteArray expectedMd5;
signals:
};

#endif // CLIENT_H
