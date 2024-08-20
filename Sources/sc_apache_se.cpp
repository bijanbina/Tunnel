#include "sc_apache_se.h"

ScApacheSe::ScApacheSe(QString name, QObject *parent):
    QObject(parent)
{
    con_name  = name; // for debug msg
    tx_server = new QTcpServer;
    rx_server = new QTcpServer;
    client    = new QTcpSocket;
    connect(rx_server, SIGNAL(newConnection()),
            this,      SLOT(rxAcceptConnection()));
    connect(tx_server, SIGNAL(newConnection()),
            this,      SLOT(txAcceptConnection()));

    rx_mapper_data       = new QSignalMapper(this);
    rx_mapper_error      = new QSignalMapper(this);
    rx_mapper_disconnect = new QSignalMapper(this);

    connect(rx_mapper_data      , SIGNAL(mapped(int)),
            this                , SLOT(rxReadyRead(int)));
    connect(rx_mapper_error     , SIGNAL(mapped(int)),
            this                , SLOT(rxDisplayError(int)));

    tx_mapper_data       = new QSignalMapper(this);
    tx_mapper_error      = new QSignalMapper(this);
    tx_mapper_disconnect = new QSignalMapper(this);

    connect(tx_mapper_error     , SIGNAL(mapped(int)),
            this                , SLOT(txDisplayError(int)));
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
    client->connectToHost(QHostAddress::LocalHost,
                          ScSetting::local_port);
    client->waitForConnected();
    if( client->isOpen()==0 )
    {
        qDebug() << "client: failed connection not opened";
        return;
    }
    client->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // readyRead
    connect(client, SIGNAL(readyRead()),
            this,   SLOT(txReadyRead()));

    // disconnected
    connect(client, SIGNAL(disconnected()),
            this,   SLOT(tcpDisconnected()));
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
        qDebug() << "RxServer failed, Error message is:"
                 << tx_server->errorString();
    }
}

void ScApacheSe::txAcceptConnection()
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

void ScApacheSe::rxAcceptConnection()
{
    if( rxPutInFree() )
    {
        return;
    }
    int new_con_id = rx_cons.length();
    rx_cons.push_back(NULL);
    rxSetupConnection(new_con_id);
}

void ScApacheSe::txDisplayError(int id)
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

void ScApacheSe::rxDisplayError(int id)
{
    QString msg = "FaApacheSe::rxError";
    //qDebug() << msg.toStdString().c_str()
    //         << rx_cons[id]->errorString()
    //         << rx_cons[id]->state();

    rx_cons[id]->close();
    //    if( cons[id]->error()==QTcpSocket::RemoteHostClosedError )
    //    {
    //    }
}

void ScApacheSe::tcpDisconnected()
{
    qDebug() << "FaApacheSe::Client ERRRROR disconnected";
}

void ScApacheSe::txReadyRead()
{
    tx_buf += client->readAll();
    if( tx_buf.isEmpty() )
    {
        return;
    }

    int split_size = 7000;
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
}

void ScApacheSe::rxReadyRead(int id)
{
    rx_buf += rx_cons[id]->readAll();
    if( rx_buf.length()==0 )
    {
        return;
    }
    qDebug() << "rxReadyRead" << id << rx_buf.length();
    client->write(rx_buf);
}

// return id in array where connection is free
int ScApacheSe::rxPutInFree()
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
int ScApacheSe::txPutInFree()
{
    int len = tx_cons.length();
    for( int i=0 ; i<len ; i++ )
    {
        if( tx_cons[i]->isOpen()==0 )
        {
            qDebug() << "txPutInFree: found" << i
                     << tx_cons[i]->state();
            tx_mapper_error->removeMappings(tx_cons[i]);
            delete tx_cons[i];
            txSetupConnection(i);

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
    QString msg = "FaApacheSe::" + con_name;
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
    qDebug() << msg.toStdString().c_str() << con_id
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

void ScApacheSe::txSetupConnection(int con_id)
{
    QTcpSocket *con = tx_server->nextPendingConnection();
    tx_cons[con_id] = con;
    con->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    quint32 ip_32 = con->peerAddress().toIPv4Address();
    QString msg = "FaApacheSe::txSetupConnection" + con_name;
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
    qDebug() << msg.toStdString().c_str() << con_id
             << tx_ipv4[con_id].toString();

    // displayError
    tx_mapper_error->setMapping(con, con_id);
    connect(con, SIGNAL(error(QAbstractSocket::SocketError)),
            tx_mapper_error, SLOT(map()));
}
