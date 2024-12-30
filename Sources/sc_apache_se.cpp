#include "sc_apache_se.h"
#include <QThread>

ScApacheSe::ScApacheSe(QObject *parent):
    QObject(parent)
{
    rx_cons    = new QUdpSocket;
    tx_server  = new ScTxServer(ScSetting::tx_port);
    dbg_rx     = new QUdpSocket;
    dbg_tx     = new ScTxServer(ScSetting::dbg_tx_port);
    ack_timer  = new QTimer;
    rx_curr_id = -1;
    read_bufs.resize(SC_MAX_PACKID+1);

    connect(tx_server, SIGNAL(init()),
            this     , SLOT(init()));

    connect(ack_timer, SIGNAL(timeout()),
            this     , SLOT  (sendAck()));
    ack_timer->start(SC_ACK_TIMEOUT);
}

ScApacheSe::~ScApacheSe()
{
    ;
}

void ScApacheSe::connectApp()
{
    client.setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // readyRead
    connect(&client, SIGNAL(readyRead()),
            this,   SLOT(txReadyRead()));

    // error
    connect(&client, SIGNAL(error(QAbstractSocket::SocketError)),
            this,    SLOT(clientError()));

    // connected
    connect(&client, SIGNAL(connected()),
            this,    SLOT(clientConnected()));

    // disconnected
    connect(&client, SIGNAL(disconnected()),
            this,    SLOT(clientDisconnected()));

    if( rx_cons->bind(QHostAddress::Any, ScSetting::rx_port) )
    {
        qDebug() << "RX created on port "
                 << ScSetting::rx_port;
        connect(rx_cons, SIGNAL(readyRead()),
                this   , SLOT(rxReadyRead()));
        connect(rx_cons,
                SIGNAL(error(QAbstractSocket::SocketError)),
                this   , SLOT(rxError()));
    }
    else
    {
        qDebug() << "RxServer failed, Error message is:"
                 << rx_cons->errorString();
    }

    if( dbg_rx->bind(QHostAddress::Any,
                     ScSetting::dbg_rx_port) )
    {
        qDebug() << "DR created on port "
                 << ScSetting::dbg_rx_port;
        connect(dbg_rx, SIGNAL(readyRead()),
                this  , SLOT(dbgRxReadyRead()));
        connect(dbg_rx,
                SIGNAL(error(QAbstractSocket::SocketError)),
                this  , SLOT(dbgRxError()));
    }
    else
    {
        qDebug() << "DbgServer failed, Error message is:"
                 << dbg_rx->errorString();
    }

    tx_server->tx_port = ScSetting::tx_port;
}

void ScApacheSe::init()
{
    reset();
    if( client.isOpen() )
    {
        client.disconnectFromHost();
        client.waitForDisconnected();
        client.connectToHost(QHostAddress::LocalHost,
                             ScSetting::local_port);
    }
    else
    {
        client.connectToHost(QHostAddress::LocalHost,
                             ScSetting::local_port);
    }
}

void ScApacheSe::reset()
{
    rx_curr_id = -1;
    tx_server->reset();
    dbg_tx->reset();
    rx_buf.clear();
    read_bufs.clear();
    read_bufs.resize(SC_MAX_PACKID+1);
}

void ScApacheSe::clientDisconnected()
{
    qDebug() << "ScApacheSe::Client disconnected----------------";
    reset();
}

void ScApacheSe::clientConnected()
{
    client.setSocketOption(QAbstractSocket::LowDelayOption, 1);
    qDebug() << "ScApacheSe::Client Connected############";
}

void ScApacheSe::clientError()
{
    qDebug() << "FaApacheSe::clientError"
             << client.errorString()
             << client.state();

    if( client.error()!=QTcpSocket::RemoteHostClosedError )
    {
        reset();
        QThread::msleep(100);
        client.connectToHost(QHostAddress::LocalHost,
                             ScSetting::local_port);
    }
}

void ScApacheSe::rxError()
{
    qDebug() << "FaApacheSe::rxError"
             << rx_cons->errorString()
             << rx_cons->state();
    if( rx_cons->error()==QTcpSocket::RemoteHostClosedError )
    {
    }
}

void ScApacheSe::rxReadyRead()
{
    QByteArray data;
    data.resize(rx_cons->pendingDatagramSize());
    QHostAddress sender_ip;
    quint16 sender_port;

    rx_cons->readDatagram(data.data(),
                          data.size(),
                          &sender_ip, &sender_port);
    rx_buf += data;

    processBuf();
}

void ScApacheSe::processBuf()
{
    while( rx_buf.contains(SC_DATA_EOP) )
    {
        QString buf_id_s = rx_buf.mid(0, SC_LEN_PACKID);
        int     buf_id   = buf_id_s.toInt();
        int     end      = rx_buf.indexOf(SC_DATA_EOP);

        // Extract the packet including the EOP marker
        read_bufs[buf_id] = rx_buf.mid(SC_LEN_PACKID,
                                       end-SC_LEN_PACKID);
        if( client.isOpen() )
        {
            QByteArray pack = getPack();
            int w = client.write(pack);
            qDebug() << "ScApacheSe::processBuf data_len:"
                     << pack.length()
                     << "buf_id:" << buf_id << rx_buf
                     << "rx_curr_id:" << rx_curr_id;
        }
        else
        {
            init();
            qDebug() << "ScApacheSe::processBuf"
                     << "client is not open"
                     << read_bufs[buf_id].length();
        }
        // Remove the processed packet from the buffer
        rx_buf.remove(0, end + strlen(SC_DATA_EOP));
    }
}

QByteArray ScApacheSe::getPack()
{
    QByteArray pack;
    int count = 0;
    while( sc_hasPacket(&read_bufs, rx_curr_id) )
    {
        rx_curr_id++;
        if( rx_curr_id>SC_MAX_PACKID )
        {
            rx_curr_id = 0;
        }
        pack += read_bufs[rx_curr_id];
        read_bufs[rx_curr_id].clear();
        count++;
        if( count>SC_MAX_PACKID )
        {
            break;
        }
    }

    qDebug() << "ScApacheSe::getPack start:"
             << rx_curr_id-count << "curr_id"
             << rx_curr_id << count;

    return pack;
}

void ScApacheSe::txReadyRead()
{
    QByteArray data = client.readAll();
    tx_server->write(data);
}

void ScApacheSe::dbgRxReadyRead()
{
    QByteArray dbg_buf;
    dbg_buf.resize(dbg_rx->pendingDatagramSize());
    QHostAddress sender_ip;
    quint16 sender_port;

    dbg_rx->readDatagram(dbg_buf.data(),
                         dbg_buf.size(),
                         &sender_ip, &sender_port);

    if( dbg_buf.isEmpty() )
    {
        return;
    }

    dbg_buf.remove(0, SC_LEN_PACKID);
    QByteArrayList cmd = dbg_buf.split(SC_CMD_EOP_CHAR);
    for( int i=0 ; i<cmd.length() ; i++ )
    {
//        qDebug() << "ScApacheSe::dbgRxReadyRead:"
//                 << "cmd" << cmd[i];

        if( cmd[i].contains(SC_CMD_ACK) )
        {
            int cmd_len = strlen(SC_CMD_ACK);
            cmd[i].remove(0, cmd_len);
            int ack_id  = cmd[i].toInt();

            if( sc_needResend(ack_id, tx_server->curr_id) )
            {
                tx_server->resendBuf(ack_id);
            }
            return;
        }
        qDebug() << "ScApacheSe::dbgRxReadyRead"
                 << cmd[i];
    }
}

void ScApacheSe::dbgRxError()
{
    if( dbg_rx->error()!=QTcpSocket::RemoteHostClosedError )
    {
        qDebug() << "ScApacheSe::dbgError"
                 << dbg_rx->errorString()
                 << dbg_rx->state();
    }
}

// check if we need to resend a packet
void ScApacheSe::sendAck()
{
    if( rx_curr_id>0 )
    {
        QByteArray msg = SC_CMD_ACK;
        msg += QString::number(rx_curr_id);
        dbg_tx->write(msg);
    }
}
