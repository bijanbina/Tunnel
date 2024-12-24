#include "sc_tx_server.h"
#include <QThread>

ScTxServer::ScTxServer(QObject *parent):
    QObject(parent)
{
    server  = new QUdpSocket;
    timer   = new QTimer;
    curr_id = -1;
    tx_buf.resize(SC_MAX_PACKID);

    connect(timer, SIGNAL(timeout()),
            this , SLOT  (writeBuf()));
    timer->start(SC_TXSERVER_TIMEOUT);
}

ScTxServer::~ScTxServer()
{

}

void ScTxServer::openPort(int port)
{
    tx_port = port;
    if( server->bind(QHostAddress::Any, tx_port) )
    {
        qDebug() << "created on port "
                 << tx_port;
    }
    else
    {
        qDebug() << "TxServer failed, Error message is:"
                 << server->errorString();
    }
}

void ScTxServer::reset()
{
    curr_id = -1;
    buf.clear();
}

void ScTxServer::txConnected()
{
    write(""); // to send buff data
}

void ScTxServer::txError()
{
    if( server->error()!=QTcpSocket::RemoteHostClosedError )
    {
        qDebug() << "ApacheSe::txError"
                 << server->errorString()
                 << server->state();
        server->close();
    }
}

void ScTxServer::writeBuf()
{
    if( buf.isEmpty() || ipv4.isNull() )
    {
        return;
    }

    QByteArray send_buf;
    int split_size = SC_MXX_PACKLEN;
    int len = split_size;
    if( buf.length()<split_size )
    {
        len = buf.length();
    }
    send_buf = buf.mid(0, len);
    tx_buf[curr_id] = sc_mkPacket(&send_buf, &curr_id);

    if( tx_port!=ScSetting::dbg_tx_port )
    {
        qDebug() << "ScTxServer::writeBuf curr_id:"
                 << curr_id << "len:" << len;
    }
    if( sendData(send_buf) )
    {
        buf.remove(0, len);
    }
}

void ScTxServer::resendBuf(int id)
{
    QByteArray send_buf;
    qDebug() << "ScTxServer::ACK"
             << curr_id;

    sendData(tx_buf[id]);
}

void ScTxServer::write(QByteArray data)
{
    buf += data;
    if( buf.length()<SC_MIN_PACKLEN )
    {
        return;
    }
    writeBuf();
}

// return 1 when sending data is successful
int ScTxServer::sendData(QByteArray send_buf)
{
    int ret = server->writeDatagram(send_buf, ipv4,
                                    tx_port);
    server->flush();
    if( ret!=send_buf.length() )
    {
        qDebug() << "ScTxServer::sendData Error"
                 << send_buf.length() << ret;
        return 0;
    }

    return 1;
}

