#include "sc_apache_se.h"
#include <QThread>

ScApacheSe::ScApacheSe(QObject *parent):
    QObject(parent)
{
    rx_server  = new QUdpSocket;
    tx_server  = new ScTxServer;
    dbg_rx     = new QUdpSocket;
    dbg_tx     = new ScMetaServer;
    ack_timer  = new QTimer;
    rx_curr_id = 0;
    rx_buf.resize(SC_PC_CONLEN);
    read_bufs.resize(SC_MAX_PACKID+1);


    rx_server->bind(QHostAddress::Any, ScSetting::rx_port);
    connect(rx_server, SIGNAL(newConnection()),
            this     , SLOT(rxConnected()));
    connect(dbg_rx   , SIGNAL(newConnection()),
            this     , SLOT(dbgRxConnected()));


    tx_mapper_data     = new QSignalMapper(this);

    connect(ack_timer, SIGNAL(timeout()),
            this     , SLOT  (sendAck()));
    ack_timer->start(SC_ACK_TIMEOUT);
}

ScApacheSe::~ScApacheSe()
{
    if( rx_cons==NULL )
    {
        return;
    }
    if( rx_cons->isOpen() )
    {
        rx_cons->close();
    }
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

    if( rx_server->bind(QHostAddress::Any, ScSetting::rx_port) )
    {
        qDebug() << "created on port "
                 << ScSetting::rx_port;
        connect(rx_server, SIGNAL(readyRead()),
                this     , SLOT(rxReadyRead()));
        connect(rx_server,
                SIGNAL(error(QAbstractSocket::SocketError)),
                this     , SLOT(rxError()));
    }
    else
    {
        qDebug() << "RxServer failed, Error message is:"
                 << rx_server->errorString();
    }

    if( dbg_rx->bind(QHostAddress::Any,
                     ScSetting::dbg_rx_port) )
    {
        qDebug() << "created on port "
                 << ScSetting::dbg_rx_port;
        connect(rx_server, SIGNAL(readyRead()),
                this     , SLOT(dbgRxReadyRead()));
        connect(rx_server,
                SIGNAL(error(QAbstractSocket::SocketError)),
                this     , SLOT(dbgRxError()));
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
    rx_buf += rx_cons->readAll();

    //    QByteArray data;
    //    data.resize(rx_server->pendingDatagramSize());
    //    QHostAddress sender_ip;
    //    quint16 sender_port;

    //    rx_server->readDatagram(data.data(),
    //                            data.size(),
    //                            &sender_ip, &sender_port);

    //    tx_server->tx_port = sender_port;
    //    pc_ip = sender_ip;
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

void ScApacheSe::rxDisconnected()
{
    if( rx_buf.length()==0 )
    {
        return;
    }

    QString buf_id_s = rx_buf.mid(0, SC_LEN_PACKID);
    int     buf_id   = buf_id_s.toInt();
    rx_buf.remove(0, SC_LEN_PACKID);
    read_bufs[buf_id] = rx_buf;
    rx_buf.clear();
    rx_buf.clear();

    if( client.isOpen() )
    {
        QByteArray pack = getPack();
        int w = client.write(pack);
        qDebug() <<"rxDisconnected"
                 << pack.length()
                 << "buf_id:" << buf_id
                 << "rx_curr_id:" << rx_curr_id;
        rx_buf.clear();
    }
    else
    {
        qDebug() << "ScApacheSe::rxDisconnected"
                 << "client is not open" << rx_buf.length();
    }
}

void ScApacheSe::txReadyRead()
{
    QByteArray data = client.readAll();
    tx_server->write(data);
}

void ScApacheSe::dbgRxReadyRead()
{
    QByteArray dbg_buf = dbg_rx->readAll();
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
            tx_server->resendBuf(ack_id);
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
