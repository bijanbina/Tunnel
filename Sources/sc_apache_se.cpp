#include "sc_apache_se.h"
#include <QThread>

ScApacheSe::ScApacheSe(QObject *parent):
    QObject(parent)
{
    rx_cons    = new QUdpSocket;
    tx_server  = new ScTxServer;
    dbg_rx     = new QUdpSocket;
    dbg_tx     = new ScMetaServer;
    ack_timer  = new QTimer;
    rx_curr_id = 0;
    read_bufs.resize(SC_MAX_PACKID+1);

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

    tx_server->openPort(ScSetting::tx_port);
    dbg_tx->openPort(ScSetting::dbg_tx_port);
}

void ScApacheSe::reset()
{
    rx_curr_id = 0;
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

    qDebug() << "ScApacheSe::rxReadyRead:"
             << data << ScSetting::rx_port;
//    pc_ip = sender_ip;
    rx_buf += data;
}

QByteArray ScApacheSe::getPack()
{
    QByteArray pack;
    int count = 0;
    while( read_bufs[rx_curr_id].length() )
    {
        pack += read_bufs[rx_curr_id];
        read_bufs[rx_curr_id].clear();
        rx_curr_id++;
        count++;
        if( rx_curr_id>SC_MAX_PACKID )
        {
            rx_curr_id = 0;
        }
        if( count>SC_MAX_PACKID )
        {
            break;
        }
    }

    qDebug() << "ScApacheSe::getPack start:"
             << rx_curr_id-count << count;

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
    dbg_buf.resize(rx_cons->pendingDatagramSize());
    QHostAddress sender_ip;
    quint16 sender_port;

    rx_cons->readDatagram(dbg_buf.data(),
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
        if( cmd[i]==SC_CMD_DISCONNECT )
        {
            // connect and reset automatically
            client.disconnectFromHost();
        }
        else if( cmd[i]==SC_CMD_INIT )
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
        else if( cmd[i].contains(SC_CMD_ACK) )
        {
            int cmd_len = strlen(SC_CMD_ACK);
            cmd[i].remove(0, cmd_len);
            int     ack_id   = cmd[i].toInt();

            if( ack_id>tx_server->curr_id )
            {
                return;
            }
            tx_server->resendBuf();
            return;
        }
        qDebug() << "ScApacheSe::dbgRxReadyRead"
                 << cmd[i];
    }
}

void ScApacheSe::dbgRxError()
{
    dbg_rx->close();
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
    QByteArray msg = SC_CMD_ACK;
    msg += QString::number(rx_curr_id);
    msg += SC_CMD_EOP;
    dbg_tx->write(msg);
}
