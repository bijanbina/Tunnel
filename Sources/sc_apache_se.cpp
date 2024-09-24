#include "sc_apache_se.h"
#include <QThread>

ScApacheSe::ScApacheSe(QObject *parent):
    QObject(parent)
{
    tx_server  = new ScTxServer;
    rx_server  = new QTcpServer;
    dbg_server = new QTcpServer;
    rx_curr_id = 0;
    rx_buf.resize(SC_PC_CONLEN);
    read_bufs.resize(SC_MAX_PACKID+1);

    connect(rx_server,  SIGNAL(newConnection()),
            this,       SLOT(rxConnected()));
    connect(dbg_server, SIGNAL(newConnection()),
            this,       SLOT(dbgConnected()));

    rx_mapper_data       = new QSignalMapper(this);
    rx_mapper_error      = new QSignalMapper(this);
    rx_mapper_disconnect = new QSignalMapper(this);

    connect(rx_mapper_data      , SIGNAL(mapped(int)),
            this                , SLOT(rxReadyRead(int)));
    connect(rx_mapper_error     , SIGNAL(mapped(int)),
            this                , SLOT(rxError(int)));
    connect(rx_mapper_disconnect, SIGNAL(mapped(int)),
            this                , SLOT(rxDisconnected(int)));

    tx_mapper_data       = new QSignalMapper(this);

    dbg_mapper_data       = new QSignalMapper(this);

    connect(dbg_mapper_data, SIGNAL(mapped(int)),
            this           , SLOT(dbgReadyRead(int)));
}

ScApacheSe::~ScApacheSe()
{
    int len = rx_cons.size();
    for( int i=0 ; i<len ; i++ )
    {
        if( rx_cons[i]==NULL )
        {
            continue;
        }
        if( rx_cons[i]->isOpen() )
        {
            rx_cons[i]->close();
        }
    }
}

void ScApacheSe::connectApp()
{
    client.setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // readyRead
    connect(&client, SIGNAL(readyRead()),
            this,   SLOT(txReadyRead()));

    // connected
    connect(&client, SIGNAL(error(QAbstractSocket::SocketError)),
            this,    SLOT(clientError()));

    // connected
    connect(&client, SIGNAL(connected()),
            this,    SLOT(clientConnected()));

    // disconnected
    connect(&client, SIGNAL(disconnected()),
            this,    SLOT(clientDisconnected()));

    if( rx_server->listen(QHostAddress::Any, ScSetting::rx_port) )
    {
        qDebug() << "created on port "
                 << ScSetting::rx_port;
    }
    else
    {
        qDebug() << "RxServer failed, Error message is:"
                 << rx_server->errorString();
    }

    if( dbg_server->listen(QHostAddress::Any, ScSetting::dbg_port) )
    {
        qDebug() << "created on port "
                 << ScSetting::dbg_port;
    }
    else
    {
        qDebug() << "DbgServer failed, Error message is:"
                 << dbg_server->errorString();
    }

    tx_server->openPort();
}

void ScApacheSe::reset()
{
    rx_curr_id = 0;
    tx_server->reset();
    rx_buf.clear();
    read_bufs.clear();
    rx_buf.resize(SC_PC_CONLEN);
    read_bufs.resize(SC_MAX_PACKID+1);
}

void ScApacheSe::clientDisconnected()
{
    qDebug() << "ScApacheSe::Client disconnected----------------";
    reset();
    client.connectToHost(QHostAddress::LocalHost,
                         ScSetting::local_port);
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

void ScApacheSe::rxConnected()
{
    if( rxPutInFree() )
    {
        return;
    }
    int new_con_id = rx_cons.length();
    rx_cons.push_back(NULL);
    rxSetupConnection(new_con_id);
}

void ScApacheSe::rxError(int id)
{
    //qDebug() << "FaApacheSe::rxError"
    //         << rx_cons[id]->errorString()
    //         << rx_cons[id]->state();

    rx_cons[id]->close();
    //    if( cons[id]->error()==QTcpSocket::RemoteHostClosedError )
    //    {
    //    }
}

void ScApacheSe::rxReadyRead(int id)
{
    rx_buf[id] += rx_cons[id]->readAll();
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
             << rx_curr_id-count << count
             << "rx_len:" << rx_cons.length()
             << "tx_len:" << tx_server->cons.length();

    return pack;
}

void ScApacheSe::rxDisconnected(int id)
{
    if( rx_buf[id].length()==0 )
    {
        return;
    }

    QString buf_id_s = rx_buf[id].mid(0, SC_LEN_PACKID);
    int     buf_id   = buf_id_s.toInt();
    rx_buf[id].remove(0, SC_LEN_PACKID);
    read_bufs[buf_id] = rx_buf[id];
    rx_buf[id].clear();

    if( client.isOpen() )
    {
        QByteArray pack = getPack();
        int w = client.write(pack);
        qDebug() << id << "rxDisconnected"
                 << pack.length() << w;
        rx_buf[id].clear();
    }
    else
    {
        qDebug() << "rxDisconnected: Conn is not open" << id
                 << rx_buf[id].length();
    }
}

// return id in array where connection is free
int  ScApacheSe::rxPutInFree()
{
    int len = rx_cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( rx_cons[i]->isOpen()==0 )
        {
            rx_mapper_data->removeMappings(rx_cons[i]);
            rx_mapper_error->removeMappings(rx_cons[i]);
            rx_mapper_disconnect->removeMappings(rx_cons[i]);
            delete rx_cons[i];
            rxSetupConnection(i);

            return 1;
        }
        else
        {
            //            qDebug() << "conn is open" << i;
        }
    }

    return 0;
}

void ScApacheSe::rxSetupConnection(int con_id)
{
    QTcpSocket *con = rx_server->nextPendingConnection();
    rx_cons[con_id] = con;
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "FaApacheSe::rxSetup";
    if( con_id<rx_ipv4.length() )
    { // put in free
        rx_ipv4[con_id] = QHostAddress(ip_32);
        msg += " refereshing connection";
    }
    else
    {
        rx_ipv4.push_back(QHostAddress(ip_32));
        msg += " accept connection";
        qDebug() << con_id << msg.toStdString().c_str()
                 << rx_ipv4[con_id].toString();
    }
    if( con_id>=rx_buf.length() )
    {
        rx_buf.push_back("");
    }

    // readyRead
    rx_mapper_data->setMapping(con, con_id);
    connect(con,            SIGNAL(readyRead()),
            rx_mapper_data, SLOT(map()));

    // displayError
    rx_mapper_error->setMapping(con, con_id);
    connect(con, SIGNAL(error(QAbstractSocket::SocketError)),
            rx_mapper_error, SLOT(map()));

    // disconnected
    rx_mapper_disconnect->setMapping(con, con_id);
    connect(con,                  SIGNAL(disconnected()),
            rx_mapper_disconnect, SLOT(map()));
}

void ScApacheSe::txReadyRead()
{
    QByteArray data = client.readAll();
    tx_server->write(data);
}

void ScApacheSe::dbgConnected()
{
    if( dbgPutInFree() )
    {
        return;
    }
    int new_con_id = dbg_cons.length();
    dbg_cons.push_back(NULL);
    dbgSetupConnection(new_con_id);
}

void ScApacheSe::dbgReadyRead(int id)
{
    QByteArray dbg_buf = dbg_cons[id]->readAll();
    if( dbg_buf.isEmpty() )
    {
        return;
    }

    dbg_buf.remove(0, SC_LEN_PACKID);
    if( dbg_buf=="client_disconnected" )
    {
        // connect and reset automatically
        client.disconnectFromHost();
    }
    if( dbg_buf=="init" )
    {
        reset();
        client.connectToHost(QHostAddress::LocalHost,
                             ScSetting::local_port);
    }
    qDebug() << "ScApacheSe::Debug"
             << dbg_buf;
}

// return id in array where connection is free
int  ScApacheSe::dbgPutInFree()
{
    int len = dbg_cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( dbg_cons[i]->isOpen()==0 )
        {
//            qDebug() << i << "dbgPutFree"
//                     << dbg_cons[i]->state();
            dbg_mapper_data->removeMappings(dbg_cons[i]);
            delete dbg_cons[i];
            dbgSetupConnection(i);

            return 1;
        }
        else
        {
            //qDebug() << "conn is open" << i;
        }
    }

    return 0;
}

void ScApacheSe::dbgSetupConnection(int con_id)
{
    QTcpSocket *con = dbg_server->nextPendingConnection();
    dbg_cons[con_id] = con;
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "ApacheSe::dbgSetup";
    if( con_id<dbg_ipv4.length() )
    { // put in free
        dbg_ipv4[con_id] = QHostAddress(ip_32);
        msg += " refreshing connection";
    }
    else
    {
        dbg_ipv4.push_back(QHostAddress(ip_32));
        msg += " accept connection";
    }
    qDebug() << con_id << msg.toStdString().c_str();

    // readyRead
    dbg_mapper_data->setMapping(con, con_id);
    connect(con,            SIGNAL(readyRead()),
            dbg_mapper_data, SLOT(map()));
}
