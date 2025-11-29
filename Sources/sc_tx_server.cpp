#include "sc_tx_server.h"
#include <QThread>

ScTxServer::ScTxServer(int port, QObject *parent):
    QObject(parent)
{
    server  = new QUdpSocket;
    timer   = new QTimer;
    curr_id = -1;
    is_dbg  =  0;
    tx_port =  0;
    tx_buf.resize(SC_MAX_PACKID+1);

    connect(timer, SIGNAL(timeout()),
            this , SLOT  (timerTick()));
    timer->start(SC_TXSERVER_TIMEOUT);
    data_counter  = 0;
    tick_counter  = 0;

    // because of NAT we don't know the ip!
    server->bind(QHostAddress::Any, port);
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
    tx_port =  0;
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
    { // print in green
        qDebug() << "\x1b[32mScApacheSe::dbgRx Retransmit"
                 << QString::number(id) + "->"
                 +  QString::number(curr_id)
                 << "packet len:"
                 << tx_buf[id].length() << "\x1b[0m";
        sendData(tx_buf[id]);
    }
    else
    {
        qDebug() << "ScTxServer::resendBuf failed,"
                 << "Packet not found:" << id;
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

// called every SC_TXSERVER_TIMEOUT msec
void ScTxServer::writeBuf()
{
    if( ipv4.isNull() )
    {
        return;
    }

    // for debug only
    int b_len = buf.length();
    int s_id  = curr_id+1;
    QByteArray send_buf;
    int count = 30; //rate control
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

    if( b_len && is_dbg==0 )
    { // print in red
        qDebug() << "\x1b[31mScTxServer::TX s_id:"
                 << s_id  << "count:" << curr_id-s_id+1
                 << "len" << b_len << "remaining:"
                 << buf.length() << "\x1b[0m";
    }
}

// return 1 when sending data is successful
// otherwise print error
int ScTxServer::sendData(QByteArray send_buf)
{
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
    // update client address
    rx_buf.clear();
    while( server->hasPendingDatagrams() )
    {
        QByteArray data;
        data.resize(server->pendingDatagramSize());
        server->readDatagram(data.data(), data.size(),
                             &ipv4, &tx_port);
        rx_buf += data;
    }
//  1 MB
//    int new_size = 1024 * 1024;
//    server->setSocketOption(QAbstractSocket::
//                            SendBufferSizeSocketOption, new_size);

    if( rx_buf==SC_CMD_START )
    {
        QHostAddress ip = QHostAddress(ipv4.toIPv4Address());
        QString ip_str = ip.toString();
        if( is_dbg==1 )
        {
            qDebug() << "DT Connect:" << ip_str << tx_port;
        }
        else
        {
            qDebug() << "TX Connect:" << ip_str << tx_port;
        }
        emit init();
    }
    else if( rx_buf.startsWith(SC_CMD_ACK) )
    {
        int cmd_len = strlen(SC_CMD_ACK);
        rx_buf.remove(0, cmd_len);
        int ack_id = rx_buf.toInt();
        int resend = sc_needResend(ack_id, curr_id);

        if( resend!=-1 )
        { // neet to retransmit as packet has been lost
            resendBuf(resend);
        }
        else if( ack_id!=curr_id )
        {
            qDebug() << "ScTxServer::RX TX_FAILURE"
                     << "curr_id:"   << curr_id
                     << "resend_id:" << resend
                     << "ack_id:"    << ack_id;
        }
        else if( ack_id!=curr_id )
        {
            qDebug() << "ScTxServer::Rx TX_FAILURE"
                     << "curr_id:"   << curr_id
                     << "resend_id:" << resend
                     << "ack_id:"    << ack_id;
        }
    }
}
