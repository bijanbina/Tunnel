#include "sc_apache_pc.h"

ScApachePC::ScApachePC(QObject *parent):
    QObject(parent)
{
    rx_curr_id       = 0;
    rc_connected     = 0;
    server = new QTcpServer;
    tx_con = new ScTxClient(ScSetting::tx_port);
    dbg_tx = new ScTxClient(ScSetting::dbg_tx_port);
    rx_refresh_timer    = new QTimer;
    dbgrx_refresh_timer = new QTimer;
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
    rx_mapper_data       = new QSignalMapper(this);
    rx_mapper_error      = new QSignalMapper(this);
    rx_mapper_disconnect = new QSignalMapper(this);

    connect(rx_mapper_data      , SIGNAL(mapped(int)),
            this                , SLOT  (rxReadyRead(int)));
    connect(rx_mapper_error     , SIGNAL(mapped(int)),
            this                , SLOT  (rxError(int)));
    connect(rx_mapper_disconnect, SIGNAL(mapped(int)),
            this                , SLOT  (rxDisconnected(int)));

    connect(rx_refresh_timer, SIGNAL(timeout()),
            this            , SLOT  (rxRefresh()));
    rx_refresh_timer->start(SC_PCSIDE_TIMEOUT);


    // dbg
    dbgrx_mapper_data       = new QSignalMapper(this);
    dbgrx_mapper_error      = new QSignalMapper(this);
    dbgrx_mapper_disconnect = new QSignalMapper(this);

    connect(dbgrx_mapper_data      , SIGNAL(mapped(int)),
            this                   , SLOT  (dbgReadyRead(int)));
    connect(dbgrx_mapper_error     , SIGNAL(mapped(int)),
            this                   , SLOT  (dbgError(int)));
    connect(dbgrx_mapper_disconnect, SIGNAL(mapped(int)),
            this                   , SLOT  (dbgDisconnected(int)));


    connect(dbgrx_refresh_timer, SIGNAL(timeout()),
            this               , SLOT  (dbgRefresh()));
    dbgrx_refresh_timer->start(SC_PCSIDE_TIMEOUT);


    connect(ack_timer, SIGNAL(timeout()),
            this     , SLOT  (sendAck()));
    ack_timer->start(SC_ACK_TIMEOUT);

    rx_buf.resize    (SC_PC_CONLEN);
    rx_clients.resize(SC_PC_CONLEN);
    read_bufs .resize(SC_MAX_PACKID+1);

    dbg_rx.resize(SC_PC_CONLEN);
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

    // rx
    for( int i=0 ; i<SC_PC_CONLEN ; i++ )
    {
        rx_clients[i] = new QTcpSocket;
        rx_clients[i]->connectToHost(ScSetting::remote_host,
                                     ScSetting::rx_port);
        rx_clients[i]->setSocketOption(
                    QAbstractSocket::LowDelayOption, 1);

        // readyRead
        rx_mapper_data->setMapping(rx_clients[i], i);
        connect(rx_clients[i] , SIGNAL(readyRead()),
                rx_mapper_data, SLOT(map()));

        // displayError
        rx_mapper_error->setMapping(rx_clients[i], i);
        connect(rx_clients[i],   SIGNAL(error(QAbstractSocket::SocketError)),
                rx_mapper_error, SLOT(map()));

        // disconnected
        rx_mapper_disconnect->setMapping(rx_clients[i], i);
        connect(rx_clients[i],        SIGNAL(disconnected()),
                rx_mapper_disconnect, SLOT(map()));
    }

    // dbg
    for( int i=0 ; i<SC_PC_CONLEN ; i++ )
    {
        dbg_rx[i] = new QTcpSocket;
        dbg_rx[i]->connectToHost(ScSetting::remote_host,
                                 ScSetting::dbg_rx_port);
        dbg_rx[i]->setSocketOption(
                    QAbstractSocket::LowDelayOption, 1);

        // readyRead
        dbgrx_mapper_data->setMapping(dbg_rx[i], i);
        connect(dbg_rx[i]        , SIGNAL(readyRead()),
                dbgrx_mapper_data, SLOT(map()));

        // displayError
        dbgrx_mapper_error->setMapping(dbg_rx[i], i);
        connect(dbg_rx[i],   SIGNAL(error(QAbstractSocket::SocketError)),
                dbgrx_mapper_error, SLOT(map()));

        // disconnected
        dbgrx_mapper_disconnect->setMapping(dbg_rx[i], i);
        connect(dbg_rx[i],        SIGNAL(disconnected()),
                dbgrx_mapper_disconnect, SLOT(map()));
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
    if( rx_buf[new_con_id].length() )
    {
        qDebug() << "saaalam" << new_con_id;
        cons[0]->write(rx_buf[new_con_id]);
        rx_buf[new_con_id].clear();
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
    rx_curr_id = 0;
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

void ScApachePC::rxReadyRead(int id)
{
    rx_buf[id] += rx_clients[id]->readAll();

    rc_connected = 1;
    if( tx_buf.length() )
    {
        tx_con->write(tx_buf);
        tx_buf.clear();
    }
}

void ScApachePC::rxError(int id)
{
    if( rx_clients[id]->error()!=
            QTcpSocket::RemoteHostClosedError )
    {
        qDebug() << id << "ScApachePC::rxError"
                 << rx_clients[id]->state()
                 << rx_clients[id]->errorString();
    }
}

void ScApachePC::rxDisconnected(int id)
{
    processBuffer(id);
    rx_clients[id]->connectToHost(ScSetting::remote_host,
                                  ScSetting::rx_port);
    rx_clients[id]->setSocketOption(
                QAbstractSocket::LowDelayOption, 1);
}

void ScApachePC::rxRefresh()
{
    int len = rx_clients.length();
    int count = 0;
    for( int i=0 ; i<len ; i++ )
    {
        if( rx_clients[i]->isOpen()==0 )
        {
            rx_clients[i]->connectToHost(ScSetting::remote_host,
                                         ScSetting::rx_port);
            count++;
        }
    }
    //    qDebug() << "rxRefresh" << count;

    if( count )
    {
        qDebug() << "ScApachePC::rxRefresh"
                 << rx_clients.length()
                 << rx_clients.length()-count;
    }
}

void ScApachePC::processBuffer(int id)
{
    if( rx_buf[id].isEmpty() )
    {
        return;
    }

    QString buf_id_s = rx_buf[id].mid(0, SC_LEN_PACKID);
    int     buf_id   = buf_id_s.toInt();
    qDebug() << id << "ScApachePC::processBuffer buf_id:"
             << buf_id << "data_len:"
             << rx_buf[id].length();
    rx_buf[id].remove(0, SC_LEN_PACKID);
    read_bufs[buf_id] = rx_buf[id];
    rx_buf[id].clear();
    int len = cons.length();
    if( len )
    {
        for( int i=0 ; i<len ; i++ )
        {
            if( cons[i]->isOpen() )
            {
                QByteArray pack = getPack();
                if( pack.isEmpty() )
                {
                    return;
                }
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
    qDebug() << "rxReadyRead:: Client is closed" << len;
}

// check if we need to resend a packet
void ScApachePC::sendAck()
{
    QByteArray msg = SC_CMD_ACK;
    msg += QString::number(rx_curr_id);
    msg += SC_CMD_EOP;
    dbg_tx->write(msg);
}

void ScApachePC::dbgReadyRead(int id)
{
    QByteArray dbg_buf = dbg_rx[id]->readAll();
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

void ScApachePC::dbgError(int id)
{
    if( dbg_rx[id]->error()!=
            QTcpSocket::RemoteHostClosedError )
    {
        qDebug() << id << "ScApachePC::rxError"
                 << dbg_rx[id]->state()
                 << dbg_rx[id]->errorString();
    }
}

void ScApachePC::dbgDisconnected(int id)
{
    dbg_rx[id]->connectToHost(ScSetting::remote_host,
                              ScSetting::dbg_rx_port);
    dbg_rx[id]->setSocketOption(
                QAbstractSocket::LowDelayOption, 1);
}

void ScApachePC::dbgRefresh()
{
    int len = dbg_rx.length();
    int count = 0;
    for( int i=0 ; i<len ; i++ )
    {
        if( dbg_rx[i]->isOpen()==0 &&
            cons[i]->state()!=QTcpSocket::ConnectingState &&
            cons[i]->state()!=QTcpSocket::ClosingState )
        {
            dbg_rx[i]->connectToHost(ScSetting::remote_host,
                                     ScSetting::dbg_rx_port);
            count++;
        }
        else if( dbg_rx[i]->state()!=
                 QAbstractSocket::ConnectedState )
        {
            qDebug() << "dbgRefresh" << dbg_rx[i]->state();
        }
    }

    if( count )
    {
        qDebug() << "ScApachePC::dbgRefresh"
                 << dbg_rx.length() << dbg_rx.length()-count;
    }
}

QByteArray ScApachePC::getPack()
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

    qDebug() << "ScApachePC::getPack start:"
             << rx_curr_id-count
             << "count:" << count << "tx_con:"
             << tx_con->cons.length();
    return pack;
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

    emit connected(con_id);
}
