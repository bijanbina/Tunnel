#include "sc_meta_server.h"
#include <QThread>

ScMetaServer::ScMetaServer(QObject *parent):
    QObject(parent)
{
    server  = new QUdpSocket;
    timer   = new QTimer;
    curr_id = -1;
    tx_buf.resize(SC_MAX_PACKID+1);

    connect(server, SIGNAL(error(QAbstractSocket::SocketError)),
            this  , SLOT(txError()));

    connect(timer, SIGNAL(timeout()),
            this , SLOT  (writeBuf()));
    timer->start(SC_TXSERVER_TIMEOUT);
}

ScMetaServer::~ScMetaServer()
{
}

void ScMetaServer::openPort(int port)
{
    tx_port = port;
    if( server->bind(QHostAddress::Any, tx_port) )
    {
        qDebug() << "TD created on port "
                 << tx_port;
    }
    else
    {
        qDebug() << "TxServer failed, Error message is:"
                 << server->errorString();
    }
}

void ScMetaServer::reset()
{
    curr_id = -1;
    conn_i  = 0;
    buf.clear();
}

void ScMetaServer::txConnected()
{
    //    qDebug() << tx_server.length() << "txAccept";
    write(""); // to send buff data
}

void ScMetaServer::txError()
{
    if( server->error()!=QTcpSocket::RemoteHostClosedError )
    {
        qDebug() << "ApacheSe::txError"
                 << server->errorString()
                 << server->state();
        server->close();
    }
}

void ScMetaServer::writeBuf()
{
    if( buf.isEmpty() || ipv4.isNull() )
    {
        return;
    }

    QByteArray send_buf;
    int split_size = SC_MXX_PACKLEN;
    if( server->isOpen() &&
        server->state()==QTcpSocket::ConnectedState )
    {
        int len = split_size;
        if( buf.length()<split_size )
        {
            len = buf.length();
        }
        send_buf = buf.mid(0, len);
        mkPacket(&send_buf);

        if( sendData(send_buf) )
        {
            buf.remove(0, len);
        }

        if( buf.length()==0 )
        {
            return;
        }
        return;
    }
}

void ScMetaServer::resendBuf(int id)
{
    QByteArray send_buf;

    if( server->isOpen() &&
        server->state()==QTcpSocket::ConnectedState )
    {
        send_buf = tx_buf[id];

        qDebug() << "ScMetaServer::resendBuf"
                 << id << curr_id;
        sendData(send_buf);
        return;
    }
    qDebug() << "ScMetaServer::resendBuf Failed";
}

void ScMetaServer::write(QByteArray data)
{
    buf += data;
    if( buf.length()<SC_MIN_PACKLEN )
    {
        return;
    }
    writeBuf();
}

void ScMetaServer::mkPacket(QByteArray *send_buf)
{
    curr_id++;
    QString tx_id = QString::number(curr_id);
    tx_id = tx_id.rightJustified(SC_LEN_PACKID, '0');
    send_buf->prepend(tx_id.toStdString().c_str());
    *send_buf += SC_DATA_EOP;
    tx_buf[curr_id] = *send_buf;
    if( curr_id>SC_MAX_PACKID )
    {
        curr_id = -1;
    }
}

// return 1 when sending data is successful
int ScMetaServer::sendData(QByteArray send_buf)
{
    int ret = server->writeDatagram(send_buf,
                                    ipv4, tx_port);
    server->flush();

    if( ret!=send_buf.length() )
    {
        qDebug() << "ScTxServer::sendData Error"
                 << send_buf.length() << ret;
        return 0;
    }

    return 1;
}


