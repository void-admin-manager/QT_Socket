#include "server.h"

ClientHandler::ClientHandler(qintptr handle, const QString &filePath, QObject *parent)
    :QObject(parent)
{
    this->filePath = filePath;
    socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::readyRead, this, &ClientHandler::readClient);
    connect(socket, &QTcpSocket::disconnected,
            this, [this] {
                    qDebug() << "已断开链接";
                    emit finished();  // 确保断开连接时触发销毁
                    deleteLater();     // 安全删除自身
                    });
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &ClientHandler::handleError);

    socket->setSocketDescriptor(handle);

    //计算MD5
    QFile file(filePath);
    if(file.open(QIODevice::ReadOnly)){
        QCryptographicHash hash(QCryptographicHash::Md5);
        if(hash.addData(&file)){
            fileMD5 = hash.result().toHex();
        }
        file.close();
    }else{
        qDebug() << "打开文件出错";
    }

    qDebug() << "初始化完成，正在等待请求";
}

void ClientHandler::readClient()
{
    //可用字节小于4
    if(socket->bytesAvailable() < 4) return;

    //读取头4字节
    QByteArray request = socket->read(4);
    if(request == "SEND"){
        sendFile();
    }
}

void ClientHandler::sendFile()
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly)){
        qWarning() << "无法打开并读取文件:" << filePath;
        socket->close();
        return;
    }

    QByteArray header;
    QDataStream ds(&header, QIODevice::WriteOnly);
    //写入数据流
    ds << fileMD5 << file.size();
    //写入文件头
    socket->write(header);
    //等待写入完成
    socket->waitForBytesWritten();

    //分块发送文件
    const qint64 chunkSize = 131072;
    while(!file.atEnd())
    {
        //每次读取固定大小
        QByteArray chunk = file.read(chunkSize);
        //记录已写入字节数
        qint64 bytesWritten = 0;
        //确保当前块chunk已经完全写入
        while(bytesWritten < chunk.size()){
            //分段写入socket
            qint64 res = socket->write(chunk.constData() + bytesWritten,
                                       chunk.size() - bytesWritten);
            if(res == -1){
                qWarning() << "写入错误：" << socket->errorString();
                file.close();
                socket->close();
                return;
            }
            bytesWritten += res;
            socket->waitForBytesWritten();
        }
    }

    file.close();
    qDebug() << "文件已发送到客户端，MD5为" << fileMD5;
    //主动关闭链接
    socket->disconnectFromHost();
    //关闭线程
    emit finished();
}

void ClientHandler::handleError(QAbstractSocket::SocketError error)
{
    qWarning() << "Socket 错误:" << error << socket->errorString();
    socket->close();
    //关闭线程
    emit finished();
}

Server::Server(const QString &filePath, QObject *parent)
    :QTcpServer(parent)
{
    this->filePath = filePath;
}

void Server::incomingConnection(qintptr handle){
    QThread *thread = new QThread;
    ClientHandler *handler = new ClientHandler(handle, filePath);
    //加入线程
    handler->moveToThread(thread);
    connect(thread, &QThread::started, handler, &ClientHandler::readClient);
    connect(handler, &ClientHandler::finished, thread, &QThread::quit);
    connect(handler, &ClientHandler::finished, handler, &ClientHandler::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}
