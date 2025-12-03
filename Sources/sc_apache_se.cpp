#include "sc_apache_se.h"
#include <QThread>

ScApacheSe::ScApacheSe(QObject *parent):
    QObject(parent)
{
    rx_cons    = new QUdpSocket;
    tx_server  = new ScTxServer(ScSetting::tx_port);
    dbg_tx     = new ScTxServer(ScSetting::dbg_tx_port);
    ack_timer  = new QTimer;
    rx_curr_id = -1;
    read_bufs.resize(SC_MAX_PACKID+1);

    // both channel fire init but init only runs
    // once when both tx & dbg are ready.
    connect(tx_server, SIGNAL(init()),
            this     , SLOT  (init()));
    connect(dbg_tx   , SIGNAL(init()),
            this     , SLOT  (init()));

    connect(ack_timer, SIGNAL(timeout()),
            this     , SLOT  (sendAck()));
    ack_timer->start(SC_ACK_TIMEOUT);
}

void ScApacheSe::connectApp()
{
    client.setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // signals
    connect(&client, SIGNAL(readyRead()),
            this,   SLOT(txReadyRead()));
    connect(&client, SIGNAL(error(QAbstractSocket::SocketError)),
            this,    SLOT(clientError()));
    connect(&client, SIGNAL(connected()),
            this,    SLOT(clientConnected()));
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
        qDebug() << "ScApacheSe::RxServer failed, Error:"
                 << rx_cons->errorString();
    }
}

void ScApacheSe::init()
{
    if( tx_server->tx_port==0 ||
        dbg_tx->tx_port==0 )
    { // wait till both channels are ready
        return;
    }

    if( client.isOpen() )
    {
        reset();
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
    QByteArray pack = getPack();
    int w = client.write(pack);
    qDebug() << "ScApacheSe::Client Connected"
             << w;
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
    while( rx_cons->hasPendingDatagrams() )
    {
        QByteArray data;
        data.resize(rx_cons->pendingDatagramSize());

        QHostAddress sender;
        quint16 sender_port;

        rx_cons->readDatagram(data.data(), data.size(),
                             &sender, &sender_port);

        rx_buf += data;
    }

    processBuf();
}

void ScApacheSe::processBuf()
{
    while( rx_buf.contains(SC_DATA_EOP) )
    {
        ScPacket p = sc_processPacket(&rx_buf, rx_curr_id);
        if( p.skip )
        {
            // skip already received packet
            continue;
        }

        // Save data to buffer
        read_bufs[p.id] = p.data;
        if( client.isValid() )
        {
            QByteArray pack = getPack();
            int w = client.write(pack);
            qDebug() << "ScApacheSe::processBuf data_len:"
                     << pack.length()
                     << "buf_id:" << p.id
                     << "rx_curr_id:" << rx_curr_id;
        }
        else
        {
            qDebug() << "ScApacheSe::processBuf"
                     << "client is not open"
                     << read_bufs[p.id].length();
        }
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

    if( count )
    {
        qDebug() << "ScApacheSe::rxPacket start:"
                 << rx_curr_id-count+1 << "count:" << count;
    }

    return pack;
}

void ScApacheSe::txReadyRead()
{
    QByteArray data = client.readAll();
    tx_server->write(data);
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
