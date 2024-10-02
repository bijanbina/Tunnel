#include "sc_apache_pc.h"

ScApachePC::ScApachePC(QObject *parent):
    QObject(parent)
{
    rx_curr_id   = 0;
    rc_connected = 0;
    server     = new QTcpServer;
    tx_con     = new ScTxClient(ScSetting::tx_port);
    dbg        = new ScTxClient(ScSetting::dbg_port);
    refresh_timer = new QTimer;
    ack_timer    = new QTimer;
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

    connect(refresh_timer, SIGNAL(timeout()),
            this         , SLOT  (rxRefresh()));
    refresh_timer->start(SC_PCSIDE_TIMEOUT);

    connect(ack_timer, SIGNAL(timeout()),
            this     , SLOT  (sendAck()));
    ack_timer->start(SC_ACK_TIMEOUT);

    rx_buf.resize    (SC_PC_CONLEN);
    rx_clients.resize(SC_PC_CONLEN);
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
    if( server->listen(QHostAddress::Any, ScSetting::local_port) )
    {
        qDebug() << "created on port "
                 << ScSetting::local_port;
    }
    else
    {
        qDebug() << "Server failed, Error message is:"
                 << server->errorString();
    }

    for( int i=0 ; i<SC_PC_CONLEN ; i++ )
    {
        rx_clients[i] = new QTcpSocket;
        rx_clients[i]->connectToHost(ScSetting::remote_host,
                                     ScSetting::rx_port);
        rx_clients[i]->setSocketOption(
                    QAbstractSocket::LowDelayOption, 1);

        // readyRead
        rx_mapper_data->setMapping(rx_clients[i], i);
        connect(rx_clients[i],  SIGNAL(readyRead()),
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
}

void ScApachePC::clientConnected()
{
    if( putInFree() )
    {
        dbg->write(SC_CMD_INIT SC_CMD_EOP);
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

    dbg->write(SC_CMD_INIT SC_CMD_EOP);
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
    dbg->write(SC_CMD_DISCONNECT SC_CMD_EOP);
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
                 << rx_clients.length() << cons.length()-count;
    }
}

void ScApachePC::processBuffer(int id)
{
    if( rx_buf[id].isEmpty() )
    {
        return;
    }

    QString buf_id_s = rx_buf[id].mid(0, SC_LEN_PACKID);
    int     buf_id       = buf_id_s.toInt();
    qDebug() << id << "ScApachePC::processBuffer buf_id:"
             << buf_id << rx_buf[id].length();
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
    dbg->write(msg);
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
