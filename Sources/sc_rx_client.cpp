#include "sc_rx_client.h"

ScRxClient::ScRxClient(QObject *parent):
    QObject(parent)
{
    curr_id       = 0;
    refresh_timer = new QTimer;

    mapper_data       = new QSignalMapper(this);
    mapper_connect    = new QSignalMapper(this);
    mapper_disconnect = new QSignalMapper(this);
    mapper_error      = new QSignalMapper(this);
    mapper_timer      = new QSignalMapper(this);

    connect(mapper_data      , SIGNAL(mapped(int)),
            this                   , SLOT  (readyRead(int)));
    connect(mapper_connect   , SIGNAL(mapped(int)),
            this                   , SLOT  (connected(int)));
    connect(mapper_disconnect, SIGNAL(mapped(int)),
            this                   , SLOT  (disconnected(int)));
    connect(mapper_error     , SIGNAL(mapped(int)),
            this                   , SLOT  (error(int)));
    connect(mapper_timer     , SIGNAL(mapped(int)),
            this                   , SLOT  (timeout(int)));
    connect(refresh_timer, SIGNAL(timeout()),
            this            , SLOT  (refresh()));
    refresh_timer->start(SC_PCSIDE_TIMEOUT);

    rx_buf.resize (SC_PC_CONLEN);
    clients.resize(SC_PC_CONLEN);
    read_bufs.resize(SC_MAX_PACKID+1);
    conn_timers.resize(SC_PC_CONLEN);
    for( int i=0; i<SC_PC_CONLEN ; i++ )
    {
        conn_timers[i] = new QTimer;
    }
}

ScRxClient::~ScRxClient()
{
    ;
}

void ScRxClient::init(int rx_port)
{
    port     = rx_port;
    is_debug = 0;
    if( port==ScSetting::dbg_rx_port )
    {
        is_debug = 0;
    }

    // dbg
    for( int i=0 ; i<SC_PC_CONLEN ; i++ )
    {
        clients[i] = new QTcpSocket;
        clients[i]->connectToHost(ScSetting::remote_host,
                                     port);
        clients[i]->setSocketOption(
                    QAbstractSocket::LowDelayOption, 1);

        // readyRead
        mapper_data->setMapping(clients[i], i);
        connect(clients[i] , SIGNAL(readyRead()),
                mapper_data, SLOT(map()));

        // displayError
        mapper_error->setMapping(clients[i], i);
        connect(clients[i],   SIGNAL(error(QAbstractSocket::SocketError)),
                mapper_error, SLOT(map()));

        // disconnected
        mapper_disconnect->setMapping(clients[i], i);
        connect(clients[i]       , SIGNAL(disconnected()),
                mapper_disconnect, SLOT(map()));

        // connected
        mapper_connect->setMapping(clients[i], i);
        connect(clients[i]    , SIGNAL(connected()),
                mapper_connect, SLOT(map()));

        // timeout
        mapper_timer->setMapping(clients[i], i);
        connect(clients[i]  , SIGNAL(timeout()),
                mapper_timer, SLOT(map()));
    }
}

void ScRxClient::readyRead(int id)
{
    if( is_debug )
    {
        dataReady(clients[id]->readAll());
    }
    else
    {
        rx_buf[id] += clients[id]->readAll();
    }
}

void ScRxClient::error(int id)
{
    if( clients[id]->error()!=QTcpSocket::RemoteHostClosedError )
    {
        qDebug() << id << "ScApachePC::rxError"
                 << clients[id]->state()
                 << clients[id]->errorString();
    }
}

void ScRxClient::disconnected(int id)
{
    if( is_debug==0 )
    {
        processBuffer(id);
    }
    clients[id]->connectToHost(ScSetting::remote_host,
                               port);
    clients[id]->setSocketOption(
                QAbstractSocket::LowDelayOption, 1);
}

void ScRxClient::refresh()
{
    int len = clients.length();
    int count = 0;
    for( int i=0 ; i<len ; i++ )
    {
        if( clients[i]->isOpen()==0 )
        {
            clients[i]->connectToHost(ScSetting::remote_host,
                                         ScSetting::rx_port);
            count++;
        }
    }
    //    qDebug() << "rxRefresh" << count;

    if( count )
    {
        qDebug() << "ScRxClient::refresh"
                 << clients.length()
                 << clients.length()-count;
    }
}

void ScRxClient::processBuffer(int id)
{
    if( rx_buf[id].isEmpty() )
    {
        return;
    }

    QString buf_id_s = rx_buf[id].mid(0, SC_LEN_PACKID);
    int     buf_id   = buf_id_s.toInt();
    qDebug() << id << "ScRxClient::processBuffer buf_id:"
             << buf_id << "data_len:" << rx_buf[id].length()
             << "start" << curr_id;
    rx_buf[id].remove(0, SC_LEN_PACKID);
    read_bufs[buf_id] = rx_buf[id];
    rx_buf[id].clear();
    QByteArray pack = getPack();
    if( pack.isEmpty() )
    {
        return;
    }
    emit dataReady(pack);
}

void ScRxClient::timeout(int id)
{
    conn_timers[id]->stop();
    clients[id]->abort();
}

QByteArray ScRxClient::getPack()
{
    QByteArray pack;
    int count = 0;
    while( read_bufs[curr_id].length() )
    {
        pack += read_bufs[curr_id];
        read_bufs[curr_id].clear();
        curr_id++;
        count++;
        if( curr_id>SC_MAX_PACKID )
        {
            curr_id = 0;
        }
        if( count>SC_MAX_PACKID )
        {
            break;
        }
    }

//    qDebug() << "ScApachePC::getPack start:"
//             << rx_curr_id-count
//             << "count:" << count << "tx_con:"
//             << tx_con->cons.length();
    return pack;
}
