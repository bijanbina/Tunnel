#include "sc_apache_te.h"
#include <QThread>

ScApacheTe::ScApacheTe(QObject *parent):
    QObject(parent)
{
    tx_server  = new ScTxServer(ScSetting::tx_port);
    tx_timer   = new QTimer;
    rx_server  = new QTcpServer;
    rxdbg_server = new QTcpServer;
    tx_curr_id = 0;
    rx_curr_id = 0;
    rx_buf.resize(SC_PC_CONLEN);
    read_bufs.resize(SC_MAX_PACKID+1);

    connect(rx_server,  SIGNAL(newConnection()),
            this,       SLOT(rxConnected()));
    connect(tx_server,  SIGNAL(newConnection()),
            this,       SLOT(txConnected()));
    connect(rxdbg_server, SIGNAL(newConnection()),
            this,       SLOT(dbgConnected()));

    rx_mapper_data       = new QSignalMapper(this);
    rx_mapper_error      = new QSignalMapper(this);
    rx_mapper_disconnect = new QSignalMapper(this);

    connect(rx_mapper_data , SIGNAL(mapped(int)),
            this           , SLOT(rxReadyRead(int)));
    connect(rx_mapper_error, SIGNAL(mapped(int)),
            this           , SLOT(rxError(int)));
    connect(rx_mapper_disconnect, SIGNAL(mapped(int)),
            this                , SLOT(rxDisconnected(int)));

    dbg_mapper_data       = new QSignalMapper(this);

    connect(dbg_mapper_data, SIGNAL(mapped(int)),
            this           , SLOT(dbgReadyRead(int)));

    connect(tx_timer, SIGNAL(timeout()),
            this    , SLOT  (txRefresh()));
    tx_timer->start(SC_TEST_TIMEOUT);
}

ScApacheTe::~ScApacheTe()
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

void ScApacheTe::connectApp()
{
    client.setSocketOption(QAbstractSocket::LowDelayOption, 1);
    qDebug() << "Ehsan say hello";
}

void ScApacheTe::reset()
{
    tx_buf.clear();
    rx_curr_id = 0;
    rx_buf.clear();
    read_bufs.clear();
    rx_buf.resize(SC_PC_CONLEN);
    read_bufs.resize(SC_MAX_PACKID+1);
}

void ScApacheTe::clientDisconnected()
{
    qDebug() << "FaApacheSe::Client disconnected----------------";
    client.connectToHost(QHostAddress::LocalHost,
                         ScSetting::local_port);
    reset();
}

void ScApacheTe::clientConnected()
{
    client.setSocketOption(QAbstractSocket::LowDelayOption, 1);
    qDebug() << "FaApacheSe::Client Connected############";
}

void ScApacheTe::clientError()
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

void ScApacheTe::rxConnected()
{
    if( rxPutInFree() )
    {
        return;
    }
    int new_con_id = rx_cons.length();
    rx_cons.push_back(NULL);
    rxSetupConnection(new_con_id);
}

void ScApacheTe::rxDisconnected(int id)
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

void ScApacheTe::rxError(int id)
{
    //qDebug() << "FaApacheSe::rxError"
    //         << rx_cons[id]->errorString()
    //         << rx_cons[id]->state();

    rx_cons[id]->close();
    //    if( cons[id]->error()==QTcpSocket::RemoteHostClosedError )
    //    {
    //    }
}

void ScApacheTe::rxReadyRead(int id)
{
    rx_buf[id] += rx_cons[id]->readAll();
}

QByteArray ScApacheTe::getPack()
{
    QByteArray pack;
    int count = 0;
    while( read_bufs[rx_curr_id].length() )
    {
        pack += read_bufs[rx_curr_id];
        read_bufs[rx_curr_id].clear();
        rx_curr_id++;
        count++;
        if( rx_curr_id>=SC_MAX_PACKID )
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
int ScApacheTe::rxPutInFree()
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

void ScApacheTe::rxSetupConnection(int con_id)
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
    }
    qDebug() << con_id << msg.toStdString().c_str()
             << rx_ipv4[con_id].toString();

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

void ScApacheTe::txRefresh()
{
    QByteArray buf;
    int buf_count = 500;
    for( int j=0 ; j<buf_count ; j++ )
    {
        buf += "<";
        buf += QString::number(j).rightJustified(5, '0');
        buf += ">";
    }
    tx_server->write(buf);
    qDebug() << "txRefresh";
}

void ScApacheTe::dbgConnected()
{
    if( dbgPutInFree() )
    {
        return;
    }
    int new_con_id = dbg_cons.length();
    dbg_cons.push_back(NULL);
    dbgSetupConnection(new_con_id);
}

void ScApacheTe::dbgReadyRead(int id)
{
    QByteArray dbg_buf = dbg_cons[id]->readAll();
    if( dbg_buf.isEmpty() )
    {
        return;
    }

    if( dbg_buf=="client_disconnected" )
    {
        client.disconnectFromHost();
    }
    qDebug() << "ScApacheTe::dbgReadyRead"
             << dbg_buf;
}

// return id in array where connection is free
int ScApacheTe::dbgPutInFree()
{
    int len = dbg_cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( dbg_cons[i]->isOpen()==0 )
        {
            qDebug() << i << "txPutFree"
                     << dbg_cons[i]->state();
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

void ScApacheTe::dbgSetupConnection(int con_id)
{
    QTcpSocket *con = rxdbg_server->nextPendingConnection();
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
