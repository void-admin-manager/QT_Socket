#include "client.h"

Client::Client(const QString &host, quint16 port,
               const QString &savePath, QObject *parent)
               : QObject{parent}, host(host), port(port), savePath(savePath)
{
    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &Client::requestFile);
    connect(socket, &QTcpSocket::readyRead, this, &Client::readData);
    connect(socket, &QTcpSocket::disconnected, this, &Client::verifyFile);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &Client::handleError);

    socket->connectToHost(host, port);
}

void Client::requestFile()
{
    qDebug() << "已连接到服务器，正在请求文件";
    //写入socket
    socket->write("SEND");
}

void Client::readData()
{
    //第一次接收文件
    if(fileSize == 0){
        //读取文件头信息 (4字节头 + MD5 + 文件size) 须与server端相同长度
        const int headerSize = sizeof(quint32) + 32 + sizeof(qint64);
        if(socket->bytesAvailable() < headerSize) return;

        QDataStream ds(socket);
        ds >> expectedMd5 >> fileSize;

        qDebug() << "已接收文件大小" << fileSize << "字节, MD5:" << expectedMd5;

        file.setFileName(savePath);
        if(!file.open(QIODevice::WriteOnly)){
            qCritical() << "无法写入文件" << savePath;
            socket->close();
            return;
        }
        return;
    }

    //写入文件数据
    QByteArray data = socket->readAll();
    file.write(data);
    receivedSize += data.size();

    //显示进度
    if (receivedSize % (1024 * 512) == 0) { // 每512KB更新一次
        double progress = (static_cast<double>(receivedSize) / fileSize) * 100;
        qDebug() << "正在下载:" << QString::number(progress, 'f', 1) << "%";
    }
}

void Client::verifyFile()
{
    file.close();
    qDebug() << "文件已接收, 正在验证MD5：";

    //计算接收文件的MD5
    QFile receivedFile(savePath);
    if(receivedFile.open(QIODevice::ReadOnly)){
        //初始化MD5
        QCryptographicHash hash(QCryptographicHash::Md5);
        if(hash.addData(&receivedFile)){
            QByteArray actualMd5 = hash.result().toHex();
            if(actualMd5 == expectedMd5){
                qDebug() << "MD5 验证通过!";
            }
            else{
                qCritical() << "MD5 验证失败!";
                qCritical() << "预期验证值:" << expectedMd5;
                qCritical() << "实际验证值:  " << actualMd5;
            }
        }
        receivedFile.close();
    }
    QCoreApplication::quit();
}

void Client::handleError(QAbstractSocket::SocketError error)
{
    qCritical() << "Socket 错误:" << error << socket->errorString();
    QCoreApplication::quit();
}
