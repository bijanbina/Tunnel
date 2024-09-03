#include "sc_apache_te.h"
#include <QThread>

ScApacheTe::ScApacheTe(QObject *parent):
    QObject(parent)
{
    tx_server  = new QTcpServer;
    tx_timer   = new QTimer;
    rx_server  = new QTcpServer;
    dbg_server = new QTcpServer;
    tx_curr_id = 0;
    connect(rx_server,  SIGNAL(newConnection()),
            this,       SLOT(rxConnected()));
    connect(tx_server,  SIGNAL(newConnection()),
            this,       SLOT(txConnected()));
    connect(dbg_server, SIGNAL(newConnection()),
            this,       SLOT(dbgConnected()));

    rx_mapper_data       = new QSignalMapper(this);
    rx_mapper_error      = new QSignalMapper(this);
    rx_mapper_disconnect = new QSignalMapper(this);

    connect(rx_mapper_data , SIGNAL(mapped(int)),
            this           , SLOT(rxReadyRead(int)));
    connect(rx_mapper_error, SIGNAL(mapped(int)),
            this           , SLOT(rxError(int)));

    tx_mapper_data       = new QSignalMapper(this);
    tx_mapper_error      = new QSignalMapper(this);
    tx_mapper_disconnect = new QSignalMapper(this);

    connect(tx_mapper_error, SIGNAL(mapped(int)),
            this           , SLOT(txError(int)));

    dbg_mapper_data       = new QSignalMapper(this);

    connect(dbg_mapper_data, SIGNAL(mapped(int)),
            this           , SLOT(dbgReadyRead(int)));

    connect(tx_timer, SIGNAL(timeout()),
            this    , SLOT  (txRefresh()));
    tx_timer->start(15000);
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
    client.connectToHost(QHostAddress::LocalHost,
                         ScSetting::local_port);
    client.waitForConnected();
    if( client.isOpen()==0 )
    {
        qDebug() << "client: failed connection not opened";
        return;
    }
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

    if( tx_server->listen(QHostAddress::Any, ScSetting::tx_port) )
    {
        qDebug() << "created on port "
                 << ScSetting::tx_port;
    }
    else
    {
        qDebug() << "TxServer failed, Error message is:"
                 << tx_server->errorString();
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
}

void ScApacheTe::txConnected()
{
    qDebug() << tx_cons.length() << "txAccept";
    if( txPutInFree() )
    {
        return;
    }
    int new_con_id = tx_cons.length();
    tx_cons.push_back(NULL);
    txSetupConnection(new_con_id);
    txReadyRead(); // to send buff data
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

void ScApacheTe::txError(int id)
{
    qDebug() << id << "ApacheSe::txError";
    if( tx_cons[id]->error()!=QTcpSocket::RemoteHostClosedError )
    {
        qDebug() << id << "ApacheSe::txError"
                 << tx_cons[id]->errorString()
                 << tx_cons[id]->state();
        tx_cons[id]->close();
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

void ScApacheTe::clientDisconnected()
{
    qDebug() << "FaApacheSe::Client disconnected----------------";
    client.connectToHost(QHostAddress::LocalHost,
                         ScSetting::local_port);
    tx_buf.clear();
    rx_buf.clear();
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
        tx_buf.clear();
        rx_buf.clear();
        QThread::msleep(100);
        client.connectToHost(QHostAddress::LocalHost,
                             ScSetting::local_port);
    }
}

void ScApacheTe::txReadyRead()
{
    tx_buf += client.readAll();
    if( tx_buf.isEmpty() )
    {
        return;
    }

    int split_size = 6990;
    int con_len = tx_cons.length();
    for( int i=0 ; i<con_len ; i++  )
    {
        if( tx_cons[i]->isOpen() )
        {
            int len = split_size;
            if( tx_buf.length()<split_size )
            {
                len = tx_buf.length();
            }

            tx_cons[i]->write(tx_buf.mid(0, len));
            tx_cons[i]->flush();
            tx_cons[i]->close();
            tx_buf.remove(0, len);

            if( tx_buf.length()==0 )
            {
                break;
            }
        }
    }

    qDebug() << "ScApacheSe::TX"
             << tx_buf.length();
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
    qDebug() << "ScApacheSe::Debug"
             << dbg_buf;
}

void ScApacheTe::rxReadyRead(int id)
{
    rx_buf += rx_cons[id]->readAll();
    if( rx_buf.length()==0 )
    {
        return;
    }

    if( client.isOpen() )
    {
        int w = client.write(rx_buf);
        qDebug() << id << "rxReadyRead"
                 << client.state()  << rx_buf
                 << rx_buf.length() << w;
        rx_buf.clear();
    }
    else
    {
        qDebug() << "rxReadyRead: Conn is not open" << id
                 << rx_buf.length();
    }
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

// return id in array where connection is free
int ScApacheTe::txPutInFree()
{
    int len = tx_cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( tx_cons[i]->isOpen()==0 )
        {
            qDebug() << i << "txPutFree"
                     << tx_cons[i]->state();
            tx_mapper_error->removeMappings(tx_cons[i]);
            delete tx_cons[i];
            txSetupConnection(i);

            return 1;
        }
        else
        {
            //qDebug() << "conn is open" << i;
        }
    }

    return 0;
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

void ScApacheTe::txSetupConnection(int con_id)
{
    QTcpSocket *con = tx_server->nextPendingConnection();
    tx_cons[con_id] = con;
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "ApacheSe::txSetup";
    if( con_id<tx_ipv4.length() )
    { // put in free
        tx_ipv4[con_id] = QHostAddress(ip_32);
        msg += " refreshing connection";
    }
    else
    {
        tx_ipv4.push_back(QHostAddress(ip_32));
        msg += " accept connection";
    }
    qDebug() << con_id << msg.toStdString().c_str();

    // displayError
    tx_mapper_error->setMapping(con, con_id);
    connect(con, SIGNAL(error(QAbstractSocket::SocketError)),
            tx_mapper_error, SLOT(map()));
}

void ScApacheTe::dbgSetupConnection(int con_id)
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

void ScApacheTe::txRefresh()
{
    int len = tx_cons.length();
    int count = 0;
    QByteArray buf;
    int buf_count = 1000;
    for( int i=0 ; i<len ; i++ )
    {
        buf.clear();
        for( int j=i*buf_count ; j<(i+1)*buf_count ; j++ )
        {
            buf += "<";
            buf += QString::number(j).rightJustified(5, '0');
            buf += ">";
        }
        if( tx_cons[i]->isOpen() )
        {
            QString tx_id = QString::number(tx_curr_id);
            tx_id = tx_id.rightJustified(3, '0');
            tx_curr_id++;
            if( tx_curr_id>SC_MAX_PACKID )
            {
                tx_curr_id = 0;
            }
            tx_cons[i]->write(tx_id.toStdString().c_str());
            tx_cons[i]->write(buf);
            tx_cons[i]->flush();
            tx_cons[i]->close();
            count++;
        }
    }
    qDebug() << "txRefresh" << count;
}
