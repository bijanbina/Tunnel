#include "sc_apache_pc.h"

ScApachePC::ScApachePC(QString name, QObject *parent):
    QObject(parent)
{
    con_name = name; // for debug msg
    server   = new QTcpServer;
    client   = new ScRemoteClient;
    dbg      = new ScRemoteClient;
    rx_timer = new QTimer;
    connect(server,    SIGNAL(newConnection()),
            this,      SLOT(clientConnected()));

    mapper_data       = new QSignalMapper(this);
    mapper_error      = new QSignalMapper(this);
    mapper_disconnect = new QSignalMapper(this);

    connect(mapper_data      , SIGNAL(mapped(int)),
            this             , SLOT(readyRead(int)));
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

    connect(rx_timer, SIGNAL(timeout()),
            this    , SLOT  (rxRefresh()));
    rx_timer->start(2000);
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

    rx_clients.resize(SC_PC_CONLEN);
    for( int i=0 ; i<SC_PC_CONLEN ; i++ )
    {
        rx_clients[i] = new QTcpSocket;
        rx_clients[i]->connectToHost(ScSetting::remote_host,
                                     ScSetting::rx_port);
        rx_clients[i]->waitForConnected();
        if( rx_clients[i]->isOpen()==0 )
        {
            qDebug() << "init: failed connection not opened";
            return;
        }
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
        return;
    }
    int new_con_id = cons.length();
    cons.push_back(NULL);
    setupConnection(new_con_id);
    if( rx_buf.length() )
    {
        cons[0]->write(rx_buf);
        rx_buf.clear();
    }
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
    dbg->buf = "client_disconnected";
    QHostAddress host(ScSetting::remote_host);
    dbg->writeBuf(host);
}

void ScApachePC::readyRead(int id)
{
    tx_buf += cons[id]->readAll();
//    qDebug() << "read_buf::" << data_rx.length();

    int split_size = 7000;
    while( tx_buf.length() )
    {
        int len = split_size;
        if( tx_buf.length()<split_size )
        {
            len = tx_buf.length();
        }
        client->buf = tx_buf.mid(0, len);
        tx_buf.remove(0, len);

        QHostAddress host(ScSetting::remote_host);
        client->writeBuf(host);
    }
}

void ScApachePC::rxReadyRead(int id)
{
    rx_buf += rx_clients[id]->readAll();
    int len = cons.length();
    if( len )
    {
        for( int i=0 ; i<len ; i++ )
        {
            if( cons[i]->isOpen() )
            {
                cons[i]->write(rx_buf);
                rx_buf.clear();
                return;
            }
        }
        qDebug() << "rxReadyRead:: Client is closed"
                 << len;
    }
}

void ScApachePC::rxError(int id)
{
    qDebug() << id << "rxError"
             << rx_clients[id]->state();
}

void ScApachePC::rxDisconnected(int id)
{
    rx_clients[id]->connectToHost(ScSetting::remote_host,
                                  ScSetting::rx_port);
    rx_clients[id]->waitForConnected();
    if( rx_clients[id]->isOpen()==0 )
    {
        return;
    }
    qDebug() << id << "rxDisconnected:: restored"
             << rx_clients[id]->state();
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
