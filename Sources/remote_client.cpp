#include "remote_client.h"

ScRemoteClient::ScRemoteClient(int port, QObject *parent):
    QObject(parent)
{
    tx_port = port;
    counter = 0;
    curr_id = 0;
    timer   = new QTimer;

    cons.resize(SC_PC_CONLEN);
    for( int i=0 ; i<SC_PC_CONLEN ; i++ )
    {
        cons[i] = new QTcpSocket;
        cons[i]->connectToHost(ScSetting::remote_host,
                               tx_port);
        if( cons[i]->isOpen()==0 )
        {
            qDebug() << "init: failed connection not opened";
            return;
        }
        cons[i]->setSocketOption(
                    QAbstractSocket::LowDelayOption, 1);
    }

    connect(timer, SIGNAL(timeout()),
            this , SLOT  (conRefresh()));
    timer->start(2000);
}

void ScRemoteClient::writeBuf()
{
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

//    qDebug() << "Network error The following error occurred"
//             << remote->errorString();
//    remote->close();
}
void ScRemoteClient::conRefresh()
{
    int len = cons.length();
    int count = 0;
    for( int i=0 ; i<len ; i++ )
    {
        if( cons[i]->isOpen()==0 )
        {
            cons[i]->connectToHost(ScSetting::remote_host,
                                   tx_port);
            count++;
        }
    }
        qDebug() << "conRefresh" << count;
}
