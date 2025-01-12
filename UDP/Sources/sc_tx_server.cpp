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
            this , SLOT  (timerTick()));
    timer->start(SC_TXSERVER_TIMEOUT);
    data_counter  = 0;
    tick_counter  = 0;

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

void ScTxServer::timerTick()
{
    tick_counter++;
    if( tick_counter>9 )
    {
        if( is_dbg==0 && data_counter>2000 )
        {
            qDebug() << "ScTxServer::timerTick load:"
                     << data_counter/1000 << "KB";
        }
        tick_counter = 0;
        data_counter = 0;
    }
    writeBuf();
}

void ScTxServer::txError()
{
//    qDebug() << "ScTxServer::txError"
//             << server->errorString()
//             << server->state();
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
    if( buf.length()<SC_MIN_PACKLEN )
    {
        return;
    }
}

void ScTxServer::writeBuf()
{
    if( ipv4.isNull() )
    {
        return;
    }

    QByteArray send_buf;
    int count = 20; //rate control
    while( buf.length() && count>0 &&
           data_counter<SC_MAX_RATE )
    {
        count--;
        int len = SC_MAX_PACKLEN;
        if( buf.length()<SC_MAX_PACKLEN )
        {
            len = buf.length();
        }
        send_buf = buf.mid(0, len);
        tx_buf[curr_id] = sc_mkPacket(&send_buf, &curr_id);

        if( sendData(send_buf) )
        {
            data_counter += send_buf.length();
            buf.remove(0, len);
        }
        else
        {
            curr_id--;
        }
    }
}

// return 1 when sending data is successful
int ScTxServer::sendData(QByteArray send_buf)
{
    server->waitForBytesWritten();
    int ret = server->writeDatagram(send_buf, ipv4,
                                    tx_port);
    if( ret!=send_buf.length() )
    {
        if( server->error()==QAbstractSocket::TemporaryError )
        {
            QThread::msleep(10);
            qDebug() << "ScTxServer::sendData Temp Error:"
                     << "Slowing down for a bit, data+len:"
                     << send_buf.length() << ret;
            return 0;
        }
        qDebug() << "ScTxServer::sendData Error, data_len:"
                 << send_buf.length() << ret
                 << server->errorString()
                 << server->error();
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
//  1 MB
    int newSize = 1024 * 1024;
    server->setSocketOption(QAbstractSocket::
                            SendBufferSizeSocketOption, newSize);
    emit init();
}
