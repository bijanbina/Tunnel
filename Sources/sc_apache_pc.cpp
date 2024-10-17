﻿#include "sc_apache_pc.h"

ScApachePC::ScApachePC(QObject *parent):
    QObject(parent)
{
    rc_connected = 0;
    server = new QTcpServer;
    tx_con = new ScTxClient(ScSetting::tx_port);
    rx_con = new ScRxClient(ScSetting::rx_port);
    dbg_tx = new ScDbgClient(ScSetting::dbg_tx_port);
    dbg_rx = new ScRxClient (ScSetting::dbg_rx_port);
    ack_timer     = new QTimer;
    connect(server, SIGNAL(newConnection()),
            this  , SLOT(clientConnected()));

    mapper_data       = new QSignalMapper(this);
    mapper_error      = new QSignalMapper(this);
    mapper_disconnect = new QSignalMapper(this);

    connect(mapper_data      , SIGNAL(mapped(int)),
            this             , SLOT(txReadyRead(int)));
    connect(mapper_error     , SIGNAL(mapped(int)),
            this             , SLOT(clientError(int)));
    connect(mapper_disconnect, SIGNAL(mapped(int)),
            this             , SLOT(clientDisconnected(int)));

    // rx
    connect(rx_con, SIGNAL(dataReady(QByteArray)),
            this  , SLOT  (rxReadyRead(QByteArray)));

    // dbg
    connect(dbg_rx, SIGNAL(dataReady(QByteArray)),
            this  , SLOT  (dbgReadyRead(QByteArray)));

    connect(ack_timer, SIGNAL(timeout()),
            this     , SLOT  (sendAck()));
    ack_timer->start(SC_ACK_TIMEOUT);

    read_bufs .resize(SC_MAX_PACKID+1);
}

ScApachePC::~ScApachePC()
{
    int len = cons.size();
    for( int i=0 ; i<len ; i++ )
    {
        if( cons[i]==NULL )
        {
            continue;
        }
        if( cons[i]->isOpen() )
        {
            cons[i]->close();
        }
    }
}

void ScApachePC::init()
{
    if( server->listen(QHostAddress::Any, ScSetting::local_port))
    {
        qDebug() << "created on port "
                 << ScSetting::local_port;
    }
    else
    {
        qDebug() << "Server failed, Error message is:"
                 << server->errorString();
    }
}

void ScApachePC::clientConnected()
{
    if( putInFree() )
    {
        dbg_tx->write(SC_CMD_INIT SC_CMD_EOP);
        return;
    }
    int new_con_id = cons.length();
    cons.push_back(NULL);
    setupConnection(new_con_id);
    if( rx_buf.length() )
    {
        qDebug() << "saaalam" << new_con_id;
        cons[0]->write(rx_buf);
        rx_buf.clear();
    }

    dbg_tx->write(SC_CMD_INIT SC_CMD_EOP);
}

void ScApachePC::clientError(int id)
{
    qDebug() << id << "clientError"
             << cons[id]->errorString()
             << cons[id]->state()
             << ipv4[id].toString();

    cons[id]->close();
}

void ScApachePC::clientDisconnected(int id)
{
    qDebug() << id << "clientDisconnected";
    dbg_tx->write(SC_CMD_DISCONNECT SC_CMD_EOP);
    rx_buf.clear();
    read_bufs.clear();
    rx_buf.resize    (SC_PC_CONLEN);
    read_bufs .resize(SC_MAX_PACKID+1);
    rx_con->curr_id = 0;
    tx_con->reset();
    rc_connected = 0;
}

void ScApachePC::txReadyRead(int id)
{
    QByteArray data = cons[id]->readAll();
    if( rc_connected )
    {
        //    qDebug() << "read_buf::" << data.length();
        tx_con->write(data);
    }
    else
    {
        tx_buf += data;
    }
}

void ScApachePC::rxReadyRead(QByteArray pack)
{
    rx_buf += pack;
    int len = cons.length();
    if( len )
    {
        for( int i=0 ; i<len ; i++ )
        {
            if( cons[i]->isOpen() )
            {
                int ret = cons[i]->write(pack);
                if( ret==0 )
                {
                    qDebug() << "FAILED PACK:" << pack << i
                             << "write:" << pack.length() << ret;
                }
                return;
            }
        }
    }

    rc_connected = 1;
    if( tx_buf.length() )
    {
        tx_con->write(tx_buf);
        tx_buf.clear();
    }
}

// check if we need to resend a packet
void ScApachePC::sendAck()
{
    //    if( cons.length() )
    //    {
    QByteArray msg = SC_CMD_ACK;
    msg += QString::number(rx_con->curr_id);
    msg += SC_CMD_EOP;
    dbg_tx->write(msg);
    //    }
}

void ScApachePC::dbgReadyRead(QByteArray data)
{
    QByteArray dbg_buf = data;
    if( dbg_buf.isEmpty() )
    {
        return;
    }

    dbg_buf.remove(0, SC_LEN_PACKID);
    QByteArrayList cmd = dbg_buf.split(SC_CMD_EOP_CHAR);
    for( int i=0 ; i<cmd.length() ; i++ )
    {
        if( cmd[i].contains(SC_CMD_ACK) )
        {
            int cmd_len = strlen(SC_CMD_ACK);
            cmd[i].remove(0, cmd_len);
            int     ack_id   = cmd[i].toInt();

            if( ack_id>tx_con->curr_id )
            {
                return;
            }
            tx_con->resendBuf(ack_id);
            qDebug() << "ScApacheSe::ACK"
                     << ack_id << tx_con->curr_id;
            return;
        }
        qDebug() << "ScApachePC::dbgReadyRead"
                 << cmd[i];
    }
}

// return id in array where connection is free
int ScApachePC::putInFree()
{
    int len = cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( cons[i]->isOpen()==0 )
        {
            mapper_data->removeMappings(cons[i]);
            mapper_error->removeMappings(cons[i]);
            mapper_disconnect->removeMappings(cons[i]);
            delete cons[i];
            setupConnection(i);

            return 1;
        }
        else
        {
            qDebug() << "conn is open" << i;
        }
    }

    return 0;
}

void ScApachePC::setupConnection(int con_id)
{
    QTcpSocket *con = server->nextPendingConnection();
    cons[con_id] = con;
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "setupConnection:: ";
    if( con_id<ipv4.length() )
    { // put in free
        ipv4[con_id] = QHostAddress(ip_32);
        msg += "refereshing connection";
    }
    else
    {
        ipv4.push_back(QHostAddress(ip_32));
        msg += "accept connection";
    }
    qDebug() << msg.toStdString().c_str() << con_id
             << ipv4[con_id].toString();

    // readyRead
    mapper_data->setMapping(con, con_id);
    connect(con, SIGNAL(readyRead()), mapper_data, SLOT(map()));

    // displayError
    mapper_error->setMapping(con, con_id);
    connect(con, SIGNAL(error(QAbstractSocket::SocketError)),
            mapper_error, SLOT(map()));

    // disconnected
    mapper_disconnect->setMapping(con, con_id);
    connect(con, SIGNAL(disconnected()),
            mapper_disconnect, SLOT(map()));
}
