#include "sc_tx_server.h"
#include <QThread>

ScTxServer::ScTxServer(int port, QObject *parent):
    QObject(parent)
{
    server  = new QUdpSocket;
    timer   = new QTimer;
    curr_id = -1;
    is_dbg  = 0;
    tx_buf.resize(SC_MAX_PACKID+1);

    connect(timer, SIGNAL(timeout()),
            this , SLOT  (writeBuf()));
    timer->start(SC_TXSERVER_TIMEOUT);

    // because of NAT we don't know the port!
    server->bind(port);
    connect(server, SIGNAL(readyRead()),
            this  , SLOT  (readyRead()));
    connect(server, SIGNAL(error(QAbstractSocket::SocketError)),
            this  , SLOT(txError()));

    if( port==ScSetting::dbg_tx_port )
    {
        is_dbg = 1;
    }
}

void ScTxServer::reset()
{
    curr_id = -1;
    buf.clear();
    tx_buf.clear();
    tx_buf.resize(SC_MAX_PACKID+1);
}

void ScTxServer::txError()
{
    qDebug() << "ScTxServer::txError"
             << server->errorString()
             << server->state();
}

void ScTxServer::resendBuf(int id)
{
    if( tx_buf[id].length() )
    {
        sendData(tx_buf[id]);
    }
}

void ScTxServer::write(QByteArray data)
{
    buf += data;
    writeBuf();
}

void ScTxServer::writeBuf()
{
    if( ipv4.isNull() )
    {
        return;
    }

    QByteArray send_buf;
    while( buf.length() )
    {
        int len = SC_MAX_PACKLEN;
        if( buf.length()<SC_MAX_PACKLEN )
        {
            len = buf.length();
        }
        send_buf = buf.mid(0, len);
        tx_buf[curr_id] = sc_mkPacket(&send_buf, &curr_id);

        if( sendData(send_buf) )
        {
            buf.remove(0, len);
        }
        else
        {
            qDebug() << "-----ScTxServer ERROR: DATA LOST-----";
            break;
        }
    }
}

// return 1 when sending data is successful
int ScTxServer::sendData(QByteArray send_buf)
{
    int ret = server->writeDatagram(send_buf, ipv4,
                                    tx_port);
    server->flush();
    if( ret!=send_buf.length() )
    {
        qDebug() << "ScTxServer::sendData Error, data_len:"
                 << send_buf.length() << ret;
        return 0;
    }

    return 1;
}

void ScTxServer::readyRead()
{
    // Just to update client address
    QByteArray data;
    data.resize(server->pendingDatagramSize());
    server->readDatagram(data.data(), data.size(),
                         &ipv4, &tx_port);

    qDebug() << "ScTxServer::dummy load start:"
             << tx_port << "is_dbg" << is_dbg;
    emit init();
}
