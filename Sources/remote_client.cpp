#include "remote_client.h"

ScRemoteClient::ScRemoteClient(int port, QObject *parent):
    QObject(parent)
{
    tx_port = port;
    counter = 0;
    remote  = new QTcpSocket();
    connect(remote, SIGNAL(disconnected()),
            this,   SLOT  (disconnected()));
    connect(remote, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
}

void ScRemoteClient::open()
{
    remote->connectToHost(QHostAddress(ScSetting::remote_host),
                          ScSetting::tx_port);
}

void ScRemoteClient::writeBuf(QHostAddress host)
{
    remote->connectToHost(host, tx_port);
    remote->waitForConnected();
    if( remote->isOpen()==0 )
    {
        qDebug() << "WriteBuf: failed connection not opened";
        return;
    }
    remote->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    QString buf_id = QString::number(counter);
    buf_id = buf_id.rightJustified(3, '0');
    counter++;
    if( counter>SC_MAX_PACKID )
    {
        counter = 0;
    }
    buf.prepend(buf_id.toStdString().c_str());

    int s = remote->write(buf);
    if( s!=buf.length() )
    {
        qDebug() << "writeBuf:" << buf.length() << s;
    }
    buf.clear();

    remote->disconnectFromHost();
    remote->waitForDisconnected();
    remote->close();
}

void ScRemoteClient::disconnected()
{
    remote->close();
    buf.clear();
//    qDebug() << "ScRemoteClient: Disconnected";
}

void ScRemoteClient::displayError(QAbstractSocket::SocketError
                                  socketError)
{
//    if( socketError==QTcpSocket::RemoteHostClosedError )
//    {
//        return;
//    }

    qDebug() << "Network error The following error occurred"
             << remote->errorString();
    remote->close();
}
