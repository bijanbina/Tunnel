#include "sc_apache_pcte.h"

ScApachePcTE::ScApachePcTE(QObject *parent):
    QObject(parent)
{
    rx_curr_id = 0;
    server     = new QTcpServer;
    tx_con     = new ScTxClient(ScSetting::tx_port);
    dbg        = new ScTxClient(ScSetting::dbg_tx_port);
    refresh_timer = new QTimer;
    tx_timer      = new QTimer;
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

    rx_buf.resize    (SC_PC_CONLEN);
    rx_clients.resize(SC_PC_CONLEN);
    read_bufs .resize(SC_MAX_PACKID+1);

    connect(tx_timer, SIGNAL(timeout()),
            this    , SLOT  (txTest()));
    tx_timer->start(SC_TEST_TIMEOUT);
}

ScApachePcTE::~ScApachePcTE()
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

void ScApachePcTE::init()
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
        connect(rx_clients[i],  SIGNAL(txReadyRead()),
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

void ScApachePcTE::clientConnected()
{
    if( putInFree() )
    {
        return;
    }
    int new_con_id = cons.length();
    cons.push_back(NULL);
    setupConnection(new_con_id);
    if( rx_buf[new_con_id].length() )
    {
        cons[0]->write(rx_buf[new_con_id]);
        rx_buf[new_con_id].clear();
    }
}

void ScApachePcTE::clientError(int id)
{
    qDebug() << id << "clientError"
             << cons[id]->errorString()
             << cons[id]->state()
             << ipv4[id].toString();

    cons[id]->close();
}

void ScApachePcTE::clientDisconnected(int id)
{
    qDebug() << id << "clientDisconnected";
    dbg->write("client_disconnected");
    rx_buf.clear();
    read_bufs.clear();
    rx_buf.resize    (SC_PC_CONLEN);
    read_bufs .resize(SC_MAX_PACKID+1);
}

void ScApachePcTE::txReadyRead(int id)
{
    QByteArray data = cons[id]->readAll();
//    qDebug() << "read_buf::" << data.length();
    tx_con->write(data);
}

void ScApachePcTE::rxReadyRead(int id)
{
    rx_buf[id] += rx_clients[id]->readAll();
}

void ScApachePcTE::rxError(int id)
{
    qDebug() << id << "rxError"
             << rx_clients[id]->state();
}

void ScApachePcTE::rxDisconnected(int id)
{
    processBuffer(id);
    rx_clients[id]->connectToHost(ScSetting::remote_host,
                                  ScSetting::rx_port);
    rx_clients[id]->setSocketOption(
                QAbstractSocket::LowDelayOption, 1);
}

void ScApachePcTE::rxRefresh()
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

void ScApachePcTE::txTest()
{
    int len = cons.length();
    int count = 0;
    QByteArray buf;
    int buf_count = 500;
    for( int i=0 ; i<len ; i++ )
    {
        buf.clear();
        for( int j=i*buf_count ; j<(i+1)*buf_count ; j++ )
        {
            buf += "<";
            buf += QString::number(j).rightJustified(5, '0');
            buf += ">";
        }
        tx_con->write(buf);
        count++;
    }
    qDebug() << "txRefresh" << count;
}

void ScApachePcTE::processBuffer(int id)
{
    QString buf_id_s = rx_buf[id].mid(0, SC_LEN_PACKID);
    int     buf_id   = buf_id_s.toInt();
    qDebug() << id << "processBuffer::" << buf_id
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
                cons[i]->write(pack);
                break;
            }
        }
    }
    else
    {
        qDebug() << "rxReadyRead:: Client is closed";
    }
}

QByteArray ScApachePcTE::getPack()
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

    return pack;
}

// return id in array where connection is free
int ScApachePcTE::putInFree()
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

void ScApachePcTE::setupConnection(int con_id)
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
    connect(con, SIGNAL(txReadyRead()), mapper_data, SLOT(map()));

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
